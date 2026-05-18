#include "DefenseManager.h"

#include "BaseMonster.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FlyingPlayerPawn.h"
#include "GameFramework/PlayerController.h"
#include "HexGridManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Model/Repository/SCTDDeckRepository.h"
#include "Model/Repository/SCTDPartsRepository.h"
#include "Model/Repository/SCTDPartDefinitionRepository.h"
#include "Model/Repository/SCTDUserRepository.h"
#include "Model/Repository/SCTDUserSaveGame.h"
#include "Misc/DateTime.h"
#include "SCTDDefenseTurret.h"

ADefenseManager::ADefenseManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADefenseManager::BeginPlay()
{
	Super::BeginPlay();

	CacheHexGridManager();
	EnsureUserRepository();
	if (bStartOnBeginPlay)
	{
		StartDefense();
	}
}

void ADefenseManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDefenseRunning)
	{
		return;
	}

	if (bLevelUpChoicePending)
	{
		return;
	}

	DefenseElapsedSeconds += DeltaSeconds;
	TickResources(DeltaSeconds);
	HandleBuildInput();
	TickConstruction(DeltaSeconds);
	if (DefenseDurationSeconds > 0.0f && DefenseElapsedSeconds >= DefenseDurationSeconds)
	{
		FinishDefense(GetAliveMonsterCount() <= 0);
		return;
	}

	TickSpawning(DeltaSeconds);
	TickDefenseResult();
}

void ADefenseManager::StartDefense()
{
	CacheHexGridManager();
	EnsureUserRepository();
	RuntimeMonsterSpawnSlots = MonsterSpawnSlots;
	TotalMonsterSpawnCount = 0;
	for (const FMonsterSpawnSlot& Slot : MonsterSpawnSlots)
	{
		if (Slot.MonsterClass && Slot.SpawnCount > 0)
		{
			TotalMonsterSpawnCount += Slot.SpawnCount;
		}
	}
	SpawnedMonsterCount = 0;
	DefenseElapsedSeconds = 0.0f;
	SpawnElapsedSeconds = MonsterSpawnIntervalSeconds;
	ScrapRemainder = 0.0f;
	CurrentScrap = InitialScrap;
	CollectedScrapThisRun = 0;
	RecoveredScrapThisRun = 0;
	ConsumedScrapThisRun = 0;
	CurrentDefenseRecordId = FGuid();
	if (UserRepository)
	{
		UserRepository->LoadOrCreate();
	}
	UserScrapBeforeResult = UserRepository ? UserRepository->GetScrap() : 0;
	TotalUserScrapAfterResult = UserScrapBeforeResult;
	DamageSummaryByType.Reset();
	DefensePlayerLevel = 1;
	CurrentLevelExperience = 0.0f;
	NextLevelExperienceRequirement = 1000.0f;
	bLevelUpChoicePending = false;
	LevelUpScrapCardOptions.Reset();
	bDefenseFinished = false;
	bDefenseVictory = false;
	bConstructionActive = false;
	bConstructionPaused = false;
	ConstructionTileIndex = INDEX_NONE;
	ConstructionPaidCost = 0;
	ConstructionElapsedSeconds = 0.0f;
	ConstructionRequiredSeconds = 0.0f;
	ConstructionTurretName.Reset();
	bDefenseRunning = true;
}

void ADefenseManager::StopDefense()
{
	bDefenseRunning = false;
}

void ADefenseManager::AddScrap(int32 ScrapAmount)
{
	if (ScrapAmount <= 0)
	{
		return;
	}

	CurrentScrap += ScrapAmount;
	CollectedScrapThisRun += ScrapAmount;
}

void ADefenseManager::RefundScrap(int32 ScrapAmount)
{
	if (ScrapAmount > 0)
	{
		CurrentScrap += ScrapAmount;
	}
}

void ADefenseManager::AddExperience(int32 ExpAmount)
{
	if (ExpAmount <= 0 || DefensePlayerLevel >= MaxDefensePlayerLevel)
	{
		return;
	}

	CurrentLevelExperience += static_cast<float>(ExpAmount);
	TryProcessLevelUp();
}

void ADefenseManager::RegisterDamageDealt(AActor* DamageCauser, float DamageAmount)
{
	if (!DamageCauser || DamageAmount <= 0.0f)
	{
		return;
	}

	const FString TypeName = GetDamageSourceTypeName(DamageCauser);
	FDefenseDamageSummaryRow& Row = DamageSummaryByType.FindOrAdd(TypeName);
	Row.TypeName = TypeName;
	Row.Count = FMath::Max(1, Row.Count);
	Row.DamageSum += DamageAmount;
}

float ADefenseManager::GetConstructionProgressRatio() const
{
	return ConstructionRequiredSeconds > 0.0f
		? FMath::Clamp(ConstructionElapsedSeconds / ConstructionRequiredSeconds, 0.0f, 1.0f)
		: 0.0f;
}

bool ADefenseManager::GetConstructionWorldLocation(FVector& OutWorldLocation) const
{
	if (!bConstructionActive || !HexGridManager || ConstructionTileIndex == INDEX_NONE)
	{
		return false;
	}

	if (!HexGridManager->GetTileWorldLocationBySlotIndex(ConstructionTileIndex, OutWorldLocation))
	{
		return false;
	}

	OutWorldLocation.Z += 220.0f;
	return true;
}

void ADefenseManager::SetSelectedBuildTurretIndex(int32 TurretIndex)
{
	SelectedBuildTurretIndex = FMath::Max(0, TurretIndex);
}

int32 ADefenseManager::GetCurrentAliveMonsterCount() const
{
	return GetAliveMonsterCount();
}

void ADefenseManager::SelectLevelUpCard(int32 CardIndex)
{
	if (!bLevelUpChoicePending || !LevelUpScrapCardOptions.IsValidIndex(CardIndex))
	{
		return;
	}

	AddScrap(LevelUpScrapCardOptions[CardIndex]);
	bLevelUpChoicePending = false;
	LevelUpScrapCardOptions.Reset();
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	TryProcessLevelUp();
}

void ADefenseManager::CacheHexGridManager()
{
	if (HexGridManager)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AHexGridManager> It(World); It; ++It)
	{
		HexGridManager = *It;
		return;
	}
}

void ADefenseManager::EnsureUserRepository()
{
	if (!UserRepository)
	{
		UserRepository = USCTDUserRepository::CreateUserRepository(this);
	}
	ConfigurePartDefinitionRepository();
}

void ADefenseManager::ConfigurePartDefinitionRepository()
{
	USCTDPartDefinitionRepository* DefinitionRepository = UserRepository ? UserRepository->GetPartDefinitionRepository() : nullptr;
	if (!DefinitionRepository)
	{
		return;
	}

	DefinitionRepository->SetDefinitionTables(BasePartDefinitionTable, WeaponPartDefinitionTable, ControlPartDefinitionTable);
	DefinitionRepository->SetOptionTable(PartOptionTable);
	DefinitionRepository->SetRarityTable(ItemRarityTable);
}

void ADefenseManager::TickResources(float DeltaSeconds)
{
	if (ScrapPerSecond <= 0.0f)
	{
		return;
	}

	ScrapRemainder += ScrapPerSecond * DeltaSeconds;
	const int32 ScrapToAdd = FMath::FloorToInt(ScrapRemainder);
	if (ScrapToAdd > 0)
	{
		AddScrap(ScrapToAdd);
		ScrapRemainder -= static_cast<float>(ScrapToAdd);
	}
}

void ADefenseManager::TickSpawning(float DeltaSeconds)
{
	if (!HasRemainingMonsterSpawns())
	{
		return;
	}

	SpawnElapsedSeconds += DeltaSeconds;
	while (SpawnElapsedSeconds >= MonsterSpawnIntervalSeconds && HasRemainingMonsterSpawns())
	{
		SpawnElapsedSeconds -= MonsterSpawnIntervalSeconds;
		SpawnRandomMonster();
	}
}

void ADefenseManager::TickDefenseResult()
{
	if (bDefenseFinished || HasRemainingMonsterSpawns())
	{
		return;
	}

	if (GetAliveMonsterCount() <= 0)
	{
		FinishDefense(true);
	}
}

void ADefenseManager::RollMonsterItemDrop(ABaseMonster* Monster, float DropRate)
{
	if (!Monster || FMath::FRand() > FMath::Clamp(DropRate, 0.0f, 1.0f))
	{
		return;
	}

	EnsureUserRepository();
	USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	USCTDPartDefinitionRepository* DefinitionRepository = UserRepository ? UserRepository->GetPartDefinitionRepository() : nullptr;
	if (!PartsRepository || !DefinitionRepository)
	{
		return;
	}

	const FSCTDItemRarityDefinitionRow RarityDefinition = DefinitionRepository->RollRarity();
	if (RarityDefinition.Rarity == ESCTDItemRarity::NoDrop)
	{
		return;
	}

	const ESCTDTurretPartType PartType = static_cast<ESCTDTurretPartType>(FMath::RandRange(0, 2));
	const TArray<FSCTDTurretPartDefinitionRow> Definitions = DefinitionRepository->GetPartDefinitionsByType(PartType);
	if (Definitions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Monster item drop skipped: no part definitions are configured for part type %d."), static_cast<int32>(PartType));
		return;
	}

	const FSCTDTurretPartDefinitionRow& SelectedDefinition = Definitions[FMath::RandRange(0, Definitions.Num() - 1)];
	const int32 OptionCount = FMath::RandRange(
		FMath::Max(0, RarityDefinition.MinOptionCount),
		FMath::Max(RarityDefinition.MinOptionCount, RarityDefinition.MaxOptionCount));

	if (PartsRepository->AddPartByDefinitionIdAndRarity(SelectedDefinition.DefinitionId, RarityDefinition.Rarity, OptionCount, RarityDefinition.DisplayColor, false).IsValid())
	{
		UserRepository->Save();
	}
}

void ADefenseManager::TickConstruction(float DeltaSeconds)
{
	if (!bConstructionActive)
	{
		return;
	}

	FHexTileSlot PlayerSlot;
	const bool bPlayerOnBuildTile = GetPlayerBuildTile(PlayerSlot) && PlayerSlot.SlotIndex == ConstructionTileIndex;
	bConstructionPaused = !bPlayerOnBuildTile;
	if (bConstructionPaused)
	{
		ConstructionElapsedSeconds = FMath::Max(0.0f, ConstructionElapsedSeconds - DeltaSeconds);
		if (ConstructionElapsedSeconds <= 0.0f)
		{
			CancelConstruction(true);
		}
		return;
	}

	ConstructionElapsedSeconds += DeltaSeconds;
	if (ConstructionElapsedSeconds >= ConstructionRequiredSeconds)
	{
		CompleteConstruction();
	}
}

void ADefenseManager::HandleBuildInput()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	const bool bSpaceDown = PlayerController->IsInputKeyDown(EKeys::SpaceBar);
	const bool bSpacePressed = bSpaceDown && !bWasSpaceDown;
	bWasSpaceDown = bSpaceDown;
	if (!bSpacePressed || bConstructionActive)
	{
		return;
	}

	FHexTileSlot BuildSlot;
	if (GetPlayerBuildTile(BuildSlot))
	{
		StartConstruction(BuildSlot);
	}
}

bool ADefenseManager::HasRemainingMonsterSpawns() const
{
	for (const FMonsterSpawnSlot& Slot : RuntimeMonsterSpawnSlots)
	{
		if (Slot.MonsterClass && Slot.SpawnCount > 0)
		{
			return true;
		}
	}

	return false;
}

int32 ADefenseManager::GetAliveMonsterCount() const
{
	int32 AliveMonsterCount = 0;
	UWorld* World = GetWorld();
	if (!World)
	{
		return AliveMonsterCount;
	}

	for (TActorIterator<ABaseMonster> It(World); It; ++It)
	{
		const ABaseMonster* Monster = *It;
		if (Monster && !Monster->IsActorBeingDestroyed() && Monster->GetCurrentHealth() > 0.0f)
		{
			AliveMonsterCount++;
		}
	}

	return AliveMonsterCount;
}

ABaseMonster* ADefenseManager::SpawnRandomMonster()
{
	TSubclassOf<ABaseMonster> MonsterClass = PickMonsterClassFromSlots();
	if (!MonsterClass)
	{
		return nullptr;
	}

	FVector SpawnLocation = GetActorLocation();
	GetRandomSpawnLocation(SpawnLocation);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;

	ABaseMonster* Monster = GetWorld() ? GetWorld()->SpawnActor<ABaseMonster>(MonsterClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters) : nullptr;
	if (!Monster)
	{
		return nullptr;
	}

	SpawnedMonsterCount++;
	return Monster;
}

AFlyingPlayerPawn* ADefenseManager::GetPlayerPawn() const
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PlayerController ? Cast<AFlyingPlayerPawn>(PlayerController->GetPawn()) : nullptr;
}

bool ADefenseManager::GetPlayerBuildTile(FHexTileSlot& OutSlot) const
{
	const AFlyingPlayerPawn* PlayerPawn = GetPlayerPawn();
	if (!PlayerPawn || !HexGridManager)
	{
		return false;
	}

	if (!HexGridManager->FindTileSlotAtWorldLocation(PlayerPawn->GetActorLocation(), OutSlot))
	{
		return false;
	}

	return OutSlot.TileType == EHexTileType::Tower && !BuiltTurretsByTileIndex.Contains(OutSlot.SlotIndex);
}

bool ADefenseManager::TryGetSelectedDeckTurret(FSCTDPreparedTurretRecord& OutTurretRecord, FSCTDOwnedTurretPartRecord& OutBasePart, FSCTDOwnedTurretPartRecord& OutWeaponPart, FSCTDOwnedTurretPartRecord& OutControlPart) const
{
	const USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!DeckRepository || !PartsRepository || !UserRepository)
	{
		return false;
	}

	const TArray<FSCTDTurretDeckRecord> Decks = DeckRepository->GetDecks();
	const int32 SelectedDeckIndex = UserRepository->GetSelectedTurretDeckIndex();
	if (!Decks.IsValidIndex(SelectedDeckIndex) || Decks[SelectedDeckIndex].Turrets.IsEmpty())
	{
		return false;
	}

	const TArray<FSCTDPreparedTurretRecord>& Turrets = Decks[SelectedDeckIndex].Turrets;
	if (!Turrets.IsValidIndex(SelectedBuildTurretIndex))
	{
		return false;
	}

	OutTurretRecord = Turrets[SelectedBuildTurretIndex];
	const bool bFoundParts = PartsRepository->FindResolvedPart(OutTurretRecord.BasePartInstanceId, OutBasePart)
		&& PartsRepository->FindResolvedPart(OutTurretRecord.WeaponPartInstanceId, OutWeaponPart)
		&& PartsRepository->FindResolvedPart(OutTurretRecord.ControlPartInstanceId, OutControlPart);
	if (!bFoundParts)
	{
		return false;
	}

	if (OutBasePart.MountType != OutWeaponPart.MountType)
	{
		return false;
	}
	return true;
}

void ADefenseManager::StartConstruction(const FHexTileSlot& BuildSlot)
{
	EnsureUserRepository();
	if (!TryGetSelectedDeckTurret(ConstructionTurretRecord, ConstructionBasePart, ConstructionWeaponPart, ConstructionControlPart))
	{
		return;
	}

	const int32 BuildCost = ConstructionBasePart.BuildCost + ConstructionWeaponPart.BuildCost + ConstructionControlPart.BuildCost;
	if (CurrentScrap < BuildCost)
	{
		return;
	}

	CurrentScrap -= BuildCost;
	ConstructionTileIndex = BuildSlot.SlotIndex;
	ConstructionPaidCost = BuildCost;
	ConstructionElapsedSeconds = 0.0f;
	ConstructionRequiredSeconds = FMath::Max(0.1f, ConstructionBasePart.BuildTimeSeconds + ConstructionWeaponPart.BuildTimeSeconds + ConstructionControlPart.BuildTimeSeconds);
	ConstructionTurretName = ConstructionTurretRecord.DisplayName.IsEmpty() ? ConstructionWeaponPart.DisplayName : ConstructionTurretRecord.DisplayName;
	bConstructionActive = true;
	bConstructionPaused = false;
}

void ADefenseManager::CompleteConstruction()
{
	if (!HexGridManager || ConstructionTileIndex == INDEX_NONE)
	{
		CancelConstruction(true);
		return;
	}

	FVector BuildLocation;
	if (!HexGridManager->GetTileWorldLocationBySlotIndex(ConstructionTileIndex, BuildLocation))
	{
		CancelConstruction(true);
		return;
	}
	BuildLocation.Z += 120.0f;

	UClass* SpawnClass = DefenseTurretClass ? DefenseTurretClass.Get() : ASCTDDefenseTurret::StaticClass();
	ASCTDDefenseTurret* Turret = GetWorld() ? GetWorld()->SpawnActor<ASCTDDefenseTurret>(SpawnClass, BuildLocation, FRotator::ZeroRotator) : nullptr;
	if (Turret)
	{
		Turret->InitializeFromRecords(ConstructionTurretRecord, ConstructionBasePart, ConstructionWeaponPart, ConstructionControlPart);
		BuiltTurretsByTileIndex.Add(ConstructionTileIndex, Turret);
		RegisterBuiltTurretType(ConstructionTurretName, ConstructionPaidCost);
		ConsumedScrapThisRun += ConstructionPaidCost;
	}
	else
	{
		CancelConstruction(true);
		return;
	}

	ClearConstructionState();
}

void ADefenseManager::CancelConstruction(bool bRefundCost)
{
	if (bRefundCost && ConstructionPaidCost > 0)
	{
		RefundScrap(ConstructionPaidCost);
	}

	ClearConstructionState();
}

void ADefenseManager::ClearConstructionState()
{
	bConstructionActive = false;
	bConstructionPaused = false;
	ConstructionTileIndex = INDEX_NONE;
	ConstructionPaidCost = 0;
	ConstructionElapsedSeconds = 0.0f;
	ConstructionRequiredSeconds = 0.0f;
	ConstructionTurretName.Reset();
	ConstructionTurretRecord = FSCTDPreparedTurretRecord();
	ConstructionBasePart = FSCTDOwnedTurretPartRecord();
	ConstructionWeaponPart = FSCTDOwnedTurretPartRecord();
	ConstructionControlPart = FSCTDOwnedTurretPartRecord();
}

void ADefenseManager::TryProcessLevelUp()
{
	if (bLevelUpChoicePending || DefensePlayerLevel >= MaxDefensePlayerLevel)
	{
		return;
	}

	if (CurrentLevelExperience < NextLevelExperienceRequirement)
	{
		return;
	}

	CurrentLevelExperience -= NextLevelExperienceRequirement;
	DefensePlayerLevel = FMath::Min(MaxDefensePlayerLevel, DefensePlayerLevel + 1);
	NextLevelExperienceRequirement *= 1.1f;
	GenerateLevelUpCardOptions();
	bLevelUpChoicePending = true;
	UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void ADefenseManager::GenerateLevelUpCardOptions()
{
	LevelUpScrapCardOptions.Reset();
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		LevelUpScrapCardOptions.Add(FMath::RandRange(50, 200));
	}
}

void ADefenseManager::RegisterBuiltTurretType(const FString& TypeName, int32 BuildCost)
{
	const FString SafeTypeName = TypeName.IsEmpty() ? TEXT("TURRET") : TypeName;
	FDefenseDamageSummaryRow& Row = DamageSummaryByType.FindOrAdd(SafeTypeName);
	Row.TypeName = SafeTypeName;
	Row.Count++;
	Row.TotalBuildCost += FMath::Max(0, BuildCost);
}

FString ADefenseManager::GetDamageSourceTypeName(AActor* DamageCauser) const
{
	if (const ASCTDDefenseTurret* Turret = Cast<ASCTDDefenseTurret>(DamageCauser))
	{
		return Turret->GetDisplayName().IsEmpty() ? TEXT("TURRET") : Turret->GetDisplayName();
	}
	if (DamageCauser && DamageCauser->IsA<AFlyingPlayerPawn>())
	{
		return TEXT("AIRCRAFT");
	}
	return DamageCauser ? DamageCauser->GetName() : TEXT("UNKNOWN");
}

TArray<FDefenseDamageSummaryRow> ADefenseManager::GetSortedDamageSummaryRows() const
{
	TArray<FDefenseDamageSummaryRow> Rows;
	DamageSummaryByType.GenerateValueArray(Rows);
	float TotalDamage = 0.0f;
	for (const FDefenseDamageSummaryRow& Row : Rows)
	{
		TotalDamage += Row.DamageSum;
	}

	for (FDefenseDamageSummaryRow& Row : Rows)
	{
		Row.DamageRatioPercent = TotalDamage > 0.0f
			? (Row.DamageSum / TotalDamage) * 100.0f
			: 0.0f;
		Row.DamagePerScrab = Row.TotalBuildCost > 0
			? Row.DamageSum / static_cast<float>(Row.TotalBuildCost)
			: 0.0f;
	}

	Rows.Sort([](const FDefenseDamageSummaryRow& A, const FDefenseDamageSummaryRow& B)
	{
		return A.DamageSum > B.DamageSum;
	});
	return Rows;
}

TArray<FDefenseRecordSummaryRow> ADefenseManager::GetDefenseRecordSummaryRows() const
{
	TArray<FDefenseRecordSummaryRow> Rows;
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return Rows;
	}

	for (int32 RecordIndex = 0; RecordIndex < SaveGame->DefenseClearRecords.Num(); ++RecordIndex)
	{
		const FSCTDDefenseClearRecord& Record = SaveGame->DefenseClearRecords[RecordIndex];

		FDefenseRecordSummaryRow Row;
		Row.Ranking = RecordIndex + 1;
		Row.ElapsedTimeText = FormatElapsedTime(Record.ElapsedSeconds);
		Row.ConsumedScrap = Record.ConsumedScrap;
		Row.Timestamp = Record.Timestamp;
		Row.bIsCurrentRecord = Record.RecordId.IsValid() && Record.RecordId == CurrentDefenseRecordId;
		Rows.Add(Row);
	}

	return Rows;
}

void ADefenseManager::UpdateDefenseClearRecords()
{
	if (!bDefenseVictory || !UserRepository)
	{
		return;
	}

	USCTDUserSaveGame* SaveGame = UserRepository->GetSaveGame();
	if (!SaveGame)
	{
		return;
	}

	FSCTDDefenseClearRecord NewRecord;
	NewRecord.RecordId = FGuid::NewGuid();
	NewRecord.ElapsedSeconds = DefenseElapsedSeconds;
	NewRecord.ConsumedScrap = ConsumedScrapThisRun;
	NewRecord.Timestamp = FDateTime::Now().ToString(TEXT("%y-%m-%d, %H:%M:%S"));

	SaveGame->DefenseClearRecords.Add(NewRecord);
	SaveGame->DefenseClearRecords.Sort([](const FSCTDDefenseClearRecord& A, const FSCTDDefenseClearRecord& B)
	{
		return A.ElapsedSeconds < B.ElapsedSeconds;
	});

	if (SaveGame->DefenseClearRecords.Num() > 3)
	{
		SaveGame->DefenseClearRecords.SetNum(3);
	}

	const bool bRecordRanked = SaveGame->DefenseClearRecords.ContainsByPredicate([&NewRecord](const FSCTDDefenseClearRecord& Record)
	{
		return Record.RecordId == NewRecord.RecordId;
	});
	CurrentDefenseRecordId = bRecordRanked ? NewRecord.RecordId : FGuid();
}

FString ADefenseManager::FormatElapsedTime(float ElapsedSeconds)
{
	const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(ElapsedSeconds));
	return FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60);
}

void ADefenseManager::FinishDefense(bool bVictory)
{
	if (bDefenseFinished)
	{
		return;
	}

	bDefenseRunning = false;
	bDefenseFinished = true;
	bDefenseVictory = bVictory;
	bLevelUpChoicePending = false;
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	RecoveredScrapThisRun = bDefenseVictory
		? CurrentScrap
		: FMath::FloorToInt(static_cast<float>(CurrentScrap) * 0.2f);
	EnsureUserRepository();
	if (UserRepository)
	{
		UserRepository->LoadOrCreate();
		UserScrapBeforeResult = UserRepository->GetScrap();
		UserRepository->AddScrap(RecoveredScrapThisRun);
		UpdateDefenseClearRecords();
		UserRepository->Save();
		TotalUserScrapAfterResult = UserRepository->GetScrap();
	}
	UE_LOG(LogTemp, Log, TEXT("Defense finished: %s"), bDefenseVictory ? TEXT("Victory") : TEXT("Defeat"));
}

TSubclassOf<ABaseMonster> ADefenseManager::PickMonsterClassFromSlots()
{
	int32 TotalRemainingCount = 0;
	for (const FMonsterSpawnSlot& Slot : RuntimeMonsterSpawnSlots)
	{
		if (Slot.MonsterClass && Slot.SpawnCount > 0)
		{
			TotalRemainingCount += Slot.SpawnCount;
		}
	}

	if (TotalRemainingCount <= 0)
	{
		return nullptr;
	}

	int32 Pick = FMath::RandRange(1, TotalRemainingCount);
	for (FMonsterSpawnSlot& Slot : RuntimeMonsterSpawnSlots)
	{
		if (!Slot.MonsterClass || Slot.SpawnCount <= 0)
		{
			continue;
		}

		Pick -= Slot.SpawnCount;
		if (Pick <= 0)
		{
			Slot.SpawnCount--;
			return Slot.MonsterClass;
		}
	}

	return nullptr;
}

bool ADefenseManager::GetRandomSpawnLocation(FVector& OutSpawnLocation) const
{
	if (!HexGridManager)
	{
		return false;
	}

	TArray<FVector> CandidateLocations;
	for (const int32 SpawnTileIndex : SpawnTileIndexList)
	{
		FVector SpawnLocation;
		if (HexGridManager->GetTileWorldLocationBySlotIndex(SpawnTileIndex, SpawnLocation))
		{
			CandidateLocations.Add(SpawnLocation);
		}
	}

	if (CandidateLocations.Num() == 0)
	{
		HexGridManager->GetEnemySpawnTileWorldLocations(CandidateLocations);
	}

	if (CandidateLocations.Num() == 0)
	{
		return false;
	}

	OutSpawnLocation = CandidateLocations[FMath::RandRange(0, CandidateLocations.Num() - 1)];
	return true;
}
