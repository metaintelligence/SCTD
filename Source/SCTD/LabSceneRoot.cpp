#include "LabSceneRoot.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "LabTurretFusionWidget.h"
#include "Model/Repository/SCTDPartsRepository.h"
#include "Model/Repository/SCTDUserSaveGame.h"
#include "Model/Repository/SCTDUserRepository.h"

ALabSceneRoot::ALabSceneRoot()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALabSceneRoot::BeginPlay()
{
	Super::BeginPlay();
	EnsureLabWidget();
}

void ALabSceneRoot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LabWidget)
	{
		LabWidget->RemoveFromParent();
		LabWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ALabSceneRoot::EnsureLabWidget()
{
	if (LabWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = bShowMouseCursor;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	UserRepository = USCTDUserRepository::CreateUserRepository(this);
	SeedMockPartsIfNeeded();

	LabWidget = CreateWidget<ULabTurretFusionWidget>(PlayerController, ULabTurretFusionWidget::StaticClass());
	if (LabWidget)
	{
		LabWidget->SetUserRepository(UserRepository);
		LabWidget->AddToViewport(100);
	}
}

void ALabSceneRoot::SeedMockPartsIfNeeded()
{
	USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!PartsRepository || !SaveGame)
	{
		return;
	}

	const auto IsLegacyMockPart = [](const FSCTDOwnedTurretPartRecord& PartRecord)
	{
		return PartRecord.DefinitionId.ToString().StartsWith(TEXT("mock_"));
	};

	const bool bHasLegacyMockParts = SaveGame->OwnedParts.ContainsByPredicate(IsLegacyMockPart);
	if (bHasLegacyMockParts)
	{
		SaveGame->OwnedParts.RemoveAll(IsLegacyMockPart);
		SaveGame->TurretDecks.Reset();
	}

	if (PartsRepository->GetOwnedPartCount() > 0)
	{
		if (bHasLegacyMockParts)
		{
			UserRepository->Save();
		}
		return;
	}

	const FLinearColor LegendaryColor(0.84f, 0.62f, 1.0f, 1.0f);
	auto AddLegendaryOption = [](FSCTDOwnedTurretPartRecord& PartRecord, FName OptionId, const FText& DisplayName, float Value)
	{
		FSCTDTurretPartOption DisplayOption;
		DisplayOption.OptionId = OptionId;
		DisplayOption.DisplayName = DisplayName;
		DisplayOption.Value = Value;
		PartRecord.AdditionalOptions.Add(DisplayOption);

		FSCTDRolledTurretPartOption RolledOption;
		RolledOption.OptionId = OptionId;
		RolledOption.Value = Value;
		PartRecord.RolledOptions.Add(RolledOption);
	};

	auto MarkLegendary = [LegendaryColor](FSCTDOwnedTurretPartRecord& PartRecord)
	{
		PartRecord.Rarity = ESCTDItemRarity::Legendary;
		PartRecord.RarityColor = LegendaryColor;
	};

	auto ApplyLegendaryMockOptions = [AddLegendaryOption](FSCTDOwnedTurretPartRecord& PartRecord)
	{
		TArray<int32> CandidateOptionIndexes;
		const int32 OptionCount = FMath::RandRange(4, 5);

		if (PartRecord.PartType == ESCTDTurretPartType::Base)
		{
			CandidateOptionIndexes = { 0, 1, 2, 3, 4, 5 };
		}
		else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
		{
			CandidateOptionIndexes = { 0, 1, 2, 3, 4 };
		}
		else
		{
			CandidateOptionIndexes = { 0, 1, 2, 3, 4 };
		}

		for (int32 Index = CandidateOptionIndexes.Num() - 1; Index > 0; --Index)
		{
			const int32 SwapIndex = FMath::RandRange(0, Index);
			CandidateOptionIndexes.Swap(Index, SwapIndex);
		}

		for (int32 Index = 0; Index < FMath::Min(OptionCount, CandidateOptionIndexes.Num()); ++Index)
		{
			const int32 OptionIndex = CandidateOptionIndexes[Index];
			const float Ratio = FMath::FRandRange(0.20f, 0.30f);
			if (PartRecord.PartType == ESCTDTurretPartType::Base)
			{
				if (OptionIndex == 0) { PartRecord.BaseHealth *= 1.0f + Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseHealth"), FText::FromString(TEXT("Health +%")), Ratio); }
				else if (OptionIndex == 1) { PartRecord.Defense *= 1.0f + Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseDefense"), FText::FromString(TEXT("Defense +%")), Ratio); }
				else if (OptionIndex == 2) { PartRecord.SelfRepairPerSecond *= 1.0f + Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseSelfRepair"), FText::FromString(TEXT("Repair +%")), Ratio); }
				else if (OptionIndex == 3) { PartRecord.AttackSpeed += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseAttackSpeed"), FText::FromString(TEXT("Attack Speed +%")), Ratio); }
				else if (OptionIndex == 4) { PartRecord.CriticalChance += FMath::FRandRange(0.10f, 0.20f); AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalChance"), FText::FromString(TEXT("Critical Chance +")), PartRecord.CriticalChance); }
				else if (OptionIndex == 5) { PartRecord.CriticalDamageMultiplier += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalDamage"), FText::FromString(TEXT("Critical Damage +")), Ratio); }
			}
			else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
			{
				if (OptionIndex == 0) { PartRecord.AttackRange += 2.0f; AddLegendaryOption(PartRecord, TEXT("IncreaseAttackRange"), FText::FromString(TEXT("Range +2")), 2.0f); }
				else if (OptionIndex == 1) { PartRecord.PhysicalDamageBonusRatio += Ratio; AddLegendaryOption(PartRecord, TEXT("PhysicalDamageBonus"), FText::FromString(TEXT("Physical Damage +%")), Ratio); }
				else if (OptionIndex == 2) { PartRecord.AttackSpeed *= 1.0f + Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseAttackSpeed"), FText::FromString(TEXT("Attack Speed +%")), Ratio); }
				else if (OptionIndex == 3) { PartRecord.AreaAttackRange += 2.0f; AddLegendaryOption(PartRecord, TEXT("IncreaseAreaRange"), FText::FromString(TEXT("Area +2")), 2.0f); }
				else if (OptionIndex == 4) { PartRecord.CriticalChance += FMath::FRandRange(0.10f, 0.20f); AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalChance"), FText::FromString(TEXT("Critical Chance +")), PartRecord.CriticalChance); }
			}
			else
			{
				if (OptionIndex == 0) { PartRecord.PhysicalDamageBonusRatio += Ratio; AddLegendaryOption(PartRecord, TEXT("PhysicalDamageBonus"), FText::FromString(TEXT("Physical Damage +%")), Ratio); }
				else if (OptionIndex == 1) { PartRecord.BaseHealth += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseHealth"), FText::FromString(TEXT("Health +%")), Ratio); }
				else if (OptionIndex == 2) { PartRecord.Defense += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseDefense"), FText::FromString(TEXT("Defense +%")), Ratio); }
				else if (OptionIndex == 3) { PartRecord.CriticalChance += FMath::FRandRange(0.10f, 0.20f); AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalChance"), FText::FromString(TEXT("Critical Chance +")), PartRecord.CriticalChance); }
				else if (OptionIndex == 4) { PartRecord.CriticalDamageMultiplier += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalDamage"), FText::FromString(TEXT("Critical Damage +")), Ratio); }
			}
		}
	};

	auto AddPart = [PartsRepository, MarkLegendary, ApplyLegendaryMockOptions](FSCTDOwnedTurretPartRecord PartRecord)
	{
		MarkLegendary(PartRecord);
		ApplyLegendaryMockOptions(PartRecord);
		PartsRepository->AddPart(PartRecord);
	};

	auto MakeBasePart = [](const TCHAR* DefinitionId, const TCHAR* DisplayName, ESCTDTurretMountType MountType, float Health, float Defense, float SelfRepair, int32 BuildCost, float BuildTime)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = ESCTDTurretPartType::Base;
		PartRecord.DisplayName = DisplayName;
		PartRecord.MountType = MountType;
		PartRecord.BaseHealth = Health;
		PartRecord.Defense = Defense;
		PartRecord.SelfRepairPerSecond = SelfRepair;
		PartRecord.BuildCost = BuildCost;
		PartRecord.BuildTimeSeconds = BuildTime;
		PartRecord.CriticalDamageMultiplier = 1.5f;
		return PartRecord;
	};

	auto MakeWeaponPart = [](const TCHAR* DefinitionId, const TCHAR* DisplayName, ESCTDTurretMountType MountType, float MinDamage, float MaxDamage, float AttackSpeed, float AttackRange, float AreaRange, float CriticalChance, float CriticalDamageBonus, float StatusChance, int32 BuildCost, float BuildTime)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = ESCTDTurretPartType::Weapon;
		PartRecord.DisplayName = DisplayName;
		PartRecord.MountType = MountType;
		PartRecord.MinAttackDamage = MinDamage;
		PartRecord.MaxAttackDamage = MaxDamage;
		PartRecord.AttackAttribute = ESCTDAttackAttribute::Physical;
		PartRecord.AttackSpeed = AttackSpeed;
		PartRecord.AttackRange = AttackRange;
		PartRecord.AreaAttackRange = AreaRange;
		PartRecord.CriticalChance = CriticalChance;
		PartRecord.CriticalDamageMultiplier = 1.0f + CriticalDamageBonus;
		PartRecord.BuildCost = BuildCost;
		PartRecord.BuildTimeSeconds = BuildTime;
		PartRecord.StatusEffectChances.Add({ ESCTDStatusEffectType::Destruction, StatusChance, 1.0f });
		PartRecord.StatusEffectChances.Add({ ESCTDStatusEffectType::Concussion, StatusChance, 1.0f });
		return PartRecord;
	};

	auto MakeControlPart = [](const TCHAR* DefinitionId, const TCHAR* DisplayName, ESCTDTargetingAI TargetingAI)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = ESCTDTurretPartType::Control;
		PartRecord.DisplayName = DisplayName;
		PartRecord.TargetingAI = TargetingAI;
		PartRecord.AIProfileId = DisplayName;
		PartRecord.CriticalDamageMultiplier = 1.5f;
		return PartRecord;
	};

	AddPart(MakeBasePart(TEXT("part_body_pylon"), TEXT("PYLON"), ESCTDTurretMountType::Tower, 400.0f, 10.0f, 1.0f, 100, 5.0f));
	AddPart(MakeBasePart(TEXT("part_body_quirass"), TEXT("QUIRASS"), ESCTDTurretMountType::Arm, 600.0f, 20.0f, 2.0f, 100, 2.0f));
	AddPart(MakeBasePart(TEXT("part_body_gunstock"), TEXT("GUNSTOCK"), ESCTDTurretMountType::Cannon, 200.0f, 10.0f, 1.0f, 100, 3.0f));

	AddPart(MakeWeaponPart(TEXT("part_weapon_mortar"), TEXT("MORTAR"), ESCTDTurretMountType::Tower, 10.0f, 20.0f, 0.33f, 5.0f, 1.0f, 0.10f, 0.50f, 0.30f, 100, 5.0f));
	AddPart(MakeWeaponPart(TEXT("part_weapon_minigun"), TEXT("MINIGUN"), ESCTDTurretMountType::Cannon, 4.0f, 6.0f, 3.0f, 3.0f, 0.0f, 0.20f, 0.50f, 0.10f, 100, 3.0f));
	AddPart(MakeWeaponPart(TEXT("part_weapon_axe"), TEXT("AXE"), ESCTDTurretMountType::Arm, 9.0f, 11.0f, 1.0f, 1.0f, 0.0f, 0.20f, 0.50f, 0.20f, 100, 2.0f));

	AddPart(MakeControlPart(TEXT("part_control_closer"), TEXT("CLOSER"), ESCTDTargetingAI::Closer));
	AddPart(MakeControlPart(TEXT("part_control_sniper"), TEXT("SNIPER"), ESCTDTargetingAI::Sniper));
	AddPart(MakeControlPart(TEXT("part_control_greedy"), TEXT("GREEDY"), ESCTDTargetingAI::Greedy));
	AddPart(MakeControlPart(TEXT("part_control_potato"), TEXT("POTATO"), ESCTDTargetingAI::Potato));
	AddPart(MakeControlPart(TEXT("part_control_chaser"), TEXT("CHASER"), ESCTDTargetingAI::Chaser));
	AddPart(MakeControlPart(TEXT("part_control_revenge"), TEXT("REVENGE"), ESCTDTargetingAI::Revenge));

	UserRepository->Save();
}
