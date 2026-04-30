#include "DefenseManager.h"

#include "BaseMonster.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HexGridManager.h"

ADefenseManager::ADefenseManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADefenseManager::BeginPlay()
{
	Super::BeginPlay();

	CacheHexGridManager();
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

	DefenseElapsedSeconds += DeltaSeconds;
	if (DefenseDurationSeconds > 0.0f && DefenseElapsedSeconds >= DefenseDurationSeconds)
	{
		StopDefense();
		return;
	}

	TickSpawning(DeltaSeconds);
}

void ADefenseManager::StartDefense()
{
	CacheHexGridManager();
	RuntimeMonsterSpawnSlots = MonsterSpawnSlots;
	DefenseElapsedSeconds = 0.0f;
	SpawnElapsedSeconds = MonsterSpawnIntervalSeconds;
	bDefenseRunning = true;
}

void ADefenseManager::StopDefense()
{
	bDefenseRunning = false;
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

	return Monster;
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
