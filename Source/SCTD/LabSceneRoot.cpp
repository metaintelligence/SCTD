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
	const auto IsSeedPartDefinition = [](const FSCTDOwnedTurretPartRecord& PartRecord)
	{
		const FString DefinitionId = PartRecord.DefinitionId.ToString();
		return DefinitionId.StartsWith(TEXT("part_body_"))
			|| DefinitionId.StartsWith(TEXT("part_weapon_"))
			|| DefinitionId.StartsWith(TEXT("part_control_"));
	};

	const auto IsSeedDamageCarrier = [](const FSCTDOwnedTurretPartRecord& PartRecord)
	{
		const FString DefinitionId = PartRecord.DefinitionId.ToString();
		return DefinitionId.StartsWith(TEXT("part_weapon_"))
			|| DefinitionId.StartsWith(TEXT("part_control_"));
	};

	const auto HasNonPhysicalDamageBonus = [](const FSCTDOwnedTurretPartRecord& PartRecord)
	{
		if (PartRecord.FireDamageBonusRatio > KINDA_SMALL_NUMBER
			|| PartRecord.LightningDamageBonusRatio > KINDA_SMALL_NUMBER
			|| PartRecord.FrostDamageBonusRatio > KINDA_SMALL_NUMBER)
		{
			return true;
		}

		const auto IsNonPhysicalDamageOption = [](FName OptionId)
		{
			return OptionId == TEXT("FireDamageBonus")
				|| OptionId == TEXT("LightningDamageBonus")
				|| OptionId == TEXT("FrostDamageBonus");
		};

		for (const FSCTDTurretPartOption& Option : PartRecord.AdditionalOptions)
		{
			if (IsNonPhysicalDamageOption(Option.OptionId))
			{
				return true;
			}
		}
		for (const FSCTDRolledTurretPartOption& Option : PartRecord.RolledOptions)
		{
			if (IsNonPhysicalDamageOption(Option.OptionId))
			{
				return true;
			}
		}
		return false;
	};

	const bool bHasSeedDamageCarrier = SaveGame->OwnedParts.ContainsByPredicate(IsSeedDamageCarrier);
	const bool bHasNonPhysicalSeedDamageBonus = SaveGame->OwnedParts.ContainsByPredicate(
		[IsSeedDamageCarrier, HasNonPhysicalDamageBonus](const FSCTDOwnedTurretPartRecord& PartRecord)
		{
			return IsSeedDamageCarrier(PartRecord) && HasNonPhysicalDamageBonus(PartRecord);
		});
	const bool bShouldRerollMissingElementalSeedDamage = bHasSeedDamageCarrier && !bHasNonPhysicalSeedDamageBonus;
	const bool bHasSeedPartMissingDescription = SaveGame->OwnedParts.ContainsByPredicate(
		[IsSeedPartDefinition](const FSCTDOwnedTurretPartRecord& PartRecord)
		{
			return IsSeedPartDefinition(PartRecord) && PartRecord.Description.IsEmpty();
		});
	constexpr int32 CurrentMockSeedVersion = 7;
	const bool bShouldRerollSeedParts = SaveGame->SaveVersion < CurrentMockSeedVersion || bShouldRerollMissingElementalSeedDamage || bHasSeedPartMissingDescription;

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

	int32 MockDamageBonusCursor = 1;
	auto ApplyLegendaryMockOptions = [AddLegendaryOption, &MockDamageBonusCursor](FSCTDOwnedTurretPartRecord& PartRecord)
	{
		TArray<int32> CandidateOptionIndexes;
		const int32 OptionCount = FMath::RandRange(4, 5);
		auto AddDamageBonusOption = [AddLegendaryOption](FSCTDOwnedTurretPartRecord& TargetPartRecord, int32 DamageBonusIndex, float Ratio)
		{
			if (DamageBonusIndex == 0)
			{
				TargetPartRecord.PhysicalDamageBonusRatio += Ratio;
				AddLegendaryOption(TargetPartRecord, TEXT("PhysicalDamageBonus"), FText::FromString(TEXT("Physical Damage +%")), Ratio);
			}
			else if (DamageBonusIndex == 1)
			{
				TargetPartRecord.FireDamageBonusRatio += Ratio;
				AddLegendaryOption(TargetPartRecord, TEXT("FireDamageBonus"), FText::FromString(TEXT("Fire Damage +%")), Ratio);
			}
			else if (DamageBonusIndex == 2)
			{
				TargetPartRecord.LightningDamageBonusRatio += Ratio;
				AddLegendaryOption(TargetPartRecord, TEXT("LightningDamageBonus"), FText::FromString(TEXT("Lightning Damage +%")), Ratio);
			}
			else
			{
				TargetPartRecord.FrostDamageBonusRatio += Ratio;
				AddLegendaryOption(TargetPartRecord, TEXT("FrostDamageBonus"), FText::FromString(TEXT("Frost Damage +%")), Ratio);
			}
		};

		if (PartRecord.PartType == ESCTDTurretPartType::Base)
		{
			CandidateOptionIndexes = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
		}
		else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
		{
			CandidateOptionIndexes = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
			if (!PartRecord.bCanAreaAttack)
			{
				CandidateOptionIndexes.Remove(6);
			}
		}
		else
		{
			CandidateOptionIndexes = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		}

		int32 RemainingOptionCount = OptionCount;
		if (PartRecord.PartType == ESCTDTurretPartType::Weapon || PartRecord.PartType == ESCTDTurretPartType::Control)
		{
			const int32 ForcedDamageBonusIndex = MockDamageBonusCursor % 4;
			++MockDamageBonusCursor;
			AddDamageBonusOption(PartRecord, ForcedDamageBonusIndex, FMath::FRandRange(0.20f, 0.30f));

			const int32 CandidateIndexToRemove = PartRecord.PartType == ESCTDTurretPartType::Weapon ? ForcedDamageBonusIndex + 1 : ForcedDamageBonusIndex;
			CandidateOptionIndexes.Remove(CandidateIndexToRemove);
			--RemainingOptionCount;
		}

		for (int32 Index = CandidateOptionIndexes.Num() - 1; Index > 0; --Index)
		{
			const int32 SwapIndex = FMath::RandRange(0, Index);
			CandidateOptionIndexes.Swap(Index, SwapIndex);
		}

		for (int32 Index = 0; Index < FMath::Min(RemainingOptionCount, CandidateOptionIndexes.Num()); ++Index)
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
				else if (OptionIndex == 6) { const int32 Reduction = FMath::CeilToInt(PartRecord.BuildCost * Ratio); PartRecord.BuildCost = FMath::Max(0, PartRecord.BuildCost - Reduction); AddLegendaryOption(PartRecord, TEXT("ReduceBuildCost"), FText::FromString(TEXT("Build Cost -%")), Ratio); }
				else if (OptionIndex == 7) { PartRecord.BuildTimeSeconds = FMath::Max(0.1f, PartRecord.BuildTimeSeconds * (1.0f - Ratio)); AddLegendaryOption(PartRecord, TEXT("ReduceBuildTime"), FText::FromString(TEXT("Build Time -%")), Ratio); }
				else if (OptionIndex == 8) { PartRecord.ScrapGainBonusRatio += Ratio; AddLegendaryOption(PartRecord, TEXT("ExtraScrapGain"), FText::FromString(TEXT("Scrap Gain +%")), Ratio); }
			}
			else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
			{
				if (OptionIndex == 0) { PartRecord.AttackRange += 2.0f; AddLegendaryOption(PartRecord, TEXT("IncreaseAttackRange"), FText::FromString(TEXT("Range +2")), 2.0f); }
				else if (OptionIndex >= 1 && OptionIndex <= 4) { AddDamageBonusOption(PartRecord, OptionIndex - 1, Ratio); }
				else if (OptionIndex == 5) { PartRecord.AttackSpeed *= 1.0f + Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseAttackSpeed"), FText::FromString(TEXT("Attack Speed +%")), Ratio); }
				else if (OptionIndex == 6 && PartRecord.bCanAreaAttack) { PartRecord.AreaAttackRange += 2.0f; AddLegendaryOption(PartRecord, TEXT("IncreaseAreaRange"), FText::FromString(TEXT("Area +2")), 2.0f); }
				else if (OptionIndex == 7) { PartRecord.CriticalChance += FMath::FRandRange(0.10f, 0.20f); AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalChance"), FText::FromString(TEXT("Critical Chance +")), PartRecord.CriticalChance); }
				else if (OptionIndex == 8) { const int32 Reduction = FMath::CeilToInt(PartRecord.BuildCost * Ratio); PartRecord.BuildCost = FMath::Max(0, PartRecord.BuildCost - Reduction); AddLegendaryOption(PartRecord, TEXT("ReduceBuildCost"), FText::FromString(TEXT("Build Cost -%")), Ratio); }
				else if (OptionIndex == 9) { PartRecord.BuildTimeSeconds = FMath::Max(0.1f, PartRecord.BuildTimeSeconds * (1.0f - Ratio)); AddLegendaryOption(PartRecord, TEXT("ReduceBuildTime"), FText::FromString(TEXT("Build Time -%")), Ratio); }
				else if (OptionIndex == 10) { PartRecord.ScrapGainBonusRatio += Ratio; AddLegendaryOption(PartRecord, TEXT("ExtraScrapGain"), FText::FromString(TEXT("Scrap Gain +%")), Ratio); }
			}
			else
			{
				if (OptionIndex >= 0 && OptionIndex <= 3) { AddDamageBonusOption(PartRecord, OptionIndex, Ratio); }
				else if (OptionIndex == 4) { PartRecord.BaseHealth += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseHealth"), FText::FromString(TEXT("Health +%")), Ratio); }
				else if (OptionIndex == 5) { PartRecord.Defense += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseDefense"), FText::FromString(TEXT("Defense +%")), Ratio); }
				else if (OptionIndex == 6) { PartRecord.CriticalChance += FMath::FRandRange(0.10f, 0.20f); AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalChance"), FText::FromString(TEXT("Critical Chance +")), PartRecord.CriticalChance); }
				else if (OptionIndex == 7) { PartRecord.CriticalDamageMultiplier += Ratio; AddLegendaryOption(PartRecord, TEXT("IncreaseCriticalDamage"), FText::FromString(TEXT("Critical Damage +")), Ratio); }
				else if (OptionIndex == 8) { const int32 Reduction = FMath::CeilToInt(PartRecord.BuildCost * Ratio); PartRecord.BuildCost = FMath::Max(0, PartRecord.BuildCost - Reduction); AddLegendaryOption(PartRecord, TEXT("ReduceBuildCost"), FText::FromString(TEXT("Build Cost -%")), Ratio); }
				else if (OptionIndex == 9) { PartRecord.BuildTimeSeconds = FMath::Max(0.1f, PartRecord.BuildTimeSeconds * (1.0f - Ratio)); AddLegendaryOption(PartRecord, TEXT("ReduceBuildTime"), FText::FromString(TEXT("Build Time -%")), Ratio); }
				else if (OptionIndex == 10) { PartRecord.ScrapGainBonusRatio += Ratio; AddLegendaryOption(PartRecord, TEXT("ExtraScrapGain"), FText::FromString(TEXT("Scrap Gain +%")), Ratio); }
			}
		}
	};

	auto BuildLegendaryPart = [MarkLegendary, ApplyLegendaryMockOptions](FSCTDOwnedTurretPartRecord PartRecord)
	{
		MarkLegendary(PartRecord);
		ApplyLegendaryMockOptions(PartRecord);
		return PartRecord;
	};

	auto MakeBasePart = [](const TCHAR* DefinitionId, const TCHAR* DisplayName, const TCHAR* Description, ESCTDTurretMountType MountType, float Health, float Defense, float SelfRepair, int32 BuildCost, float BuildTime)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = ESCTDTurretPartType::Base;
		PartRecord.DisplayName = DisplayName;
		PartRecord.Description = Description;
		PartRecord.MountType = MountType;
		PartRecord.BaseHealth = Health;
		PartRecord.Defense = Defense;
		PartRecord.SelfRepairPerSecond = SelfRepair;
		PartRecord.BuildCost = BuildCost;
		PartRecord.BuildTimeSeconds = BuildTime;
		PartRecord.CriticalDamageMultiplier = 1.5f;
		return PartRecord;
	};

	auto MakeWeaponPart = [](const TCHAR* DefinitionId, const TCHAR* DisplayName, const TCHAR* Description, ESCTDTurretMountType MountType, float MinDamage, float MaxDamage, float AttackSpeed, float AttackRange, float AreaRange, bool bCanAreaAttack, float CriticalChance, float CriticalDamageBonus, float StatusChance, int32 BuildCost, float BuildTime)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = ESCTDTurretPartType::Weapon;
		PartRecord.DisplayName = DisplayName;
		PartRecord.Description = Description;
		PartRecord.MountType = MountType;
		PartRecord.MinAttackDamage = MinDamage;
		PartRecord.MaxAttackDamage = MaxDamage;
		PartRecord.AttackAttribute = ESCTDAttackAttribute::Physical;
		PartRecord.AttackSpeed = AttackSpeed;
		PartRecord.AttackRange = AttackRange;
		PartRecord.bCanAreaAttack = bCanAreaAttack;
		PartRecord.AreaAttackRange = bCanAreaAttack ? AreaRange : 0.0f;
		PartRecord.CriticalChance = CriticalChance;
		PartRecord.CriticalDamageMultiplier = 1.0f + CriticalDamageBonus;
		PartRecord.BuildCost = BuildCost;
		PartRecord.BuildTimeSeconds = BuildTime;
		PartRecord.StatusEffectChances.Add({ ESCTDStatusEffectType::Destruction, StatusChance, 1.0f });
		PartRecord.StatusEffectChances.Add({ ESCTDStatusEffectType::Concussion, StatusChance, 1.0f });
		return PartRecord;
	};

	auto MakeControlPart = [](const TCHAR* DefinitionId, const TCHAR* DisplayName, const TCHAR* Description, ESCTDTargetingAI TargetingAI)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = ESCTDTurretPartType::Control;
		PartRecord.DisplayName = DisplayName;
		PartRecord.Description = Description;
		PartRecord.TargetingAI = TargetingAI;
		PartRecord.AIProfileId = DisplayName;
		PartRecord.CriticalDamageMultiplier = 1.5f;
		return PartRecord;
	};

	TArray<FSCTDOwnedTurretPartRecord> SeedParts;
	SeedParts.Add(BuildLegendaryPart(MakeBasePart(TEXT("part_body_pylon"), TEXT("PYLON"), TEXT("저가형 파일런은 타워형 무기를 탑재할 수 있다."), ESCTDTurretMountType::Tower, 400.0f, 10.0f, 1.0f, 100, 5.0f)));
	SeedParts.Add(BuildLegendaryPart(MakeBasePart(TEXT("part_body_quirass"), TEXT("QUIRASS"), TEXT("저가형 갑옷으로 양팔형 무기를 탑재할 수 있다."), ESCTDTurretMountType::Arm, 600.0f, 20.0f, 2.0f, 100, 2.0f)));
	SeedParts.Add(BuildLegendaryPart(MakeBasePart(TEXT("part_body_gunstock"), TEXT("GUNSTOCK"), TEXT("저가형 개머리판으로 캐논형 무기를 탑재할 수 있다."), ESCTDTurretMountType::Cannon, 200.0f, 10.0f, 1.0f, 100, 3.0f)));

	SeedParts.Add(BuildLegendaryPart(MakeWeaponPart(TEXT("part_weapon_mortar"), TEXT("MORTAR"), TEXT("느리지만 장거리 광역 공격이 가능한 박격포이다."), ESCTDTurretMountType::Tower, 10.0f, 20.0f, 0.33f, 5.0f, 1.0f, true, 0.10f, 0.50f, 0.30f, 100, 5.0f)));
	SeedParts.Add(BuildLegendaryPart(MakeWeaponPart(TEXT("part_weapon_minigun"), TEXT("MINIGUN"), TEXT("중거리 대응 사격에 탁월한 미니건이다."), ESCTDTurretMountType::Cannon, 4.0f, 6.0f, 3.0f, 3.0f, 0.0f, false, 0.20f, 0.50f, 0.10f, 100, 3.0f)));
	SeedParts.Add(BuildLegendaryPart(MakeWeaponPart(TEXT("part_weapon_axe"), TEXT("AXE"), TEXT("강력한 근거리 공격으로 무엇도 놓치지 않는 도끼이다."), ESCTDTurretMountType::Arm, 9.0f, 11.0f, 1.0f, 1.0f, 0.0f, false, 0.20f, 0.50f, 0.20f, 100, 2.0f)));

	SeedParts.Add(BuildLegendaryPart(MakeControlPart(TEXT("part_control_closer"), TEXT("CLOSER"), TEXT("가장 가까운 몬스터를 공격하는 AI"), ESCTDTargetingAI::Closer)));
	SeedParts.Add(BuildLegendaryPart(MakeControlPart(TEXT("part_control_sniper"), TEXT("SNIPER"), TEXT("가장 먼 몬스터를 공격하는 AI"), ESCTDTargetingAI::Sniper)));
	SeedParts.Add(BuildLegendaryPart(MakeControlPart(TEXT("part_control_greedy"), TEXT("GREEDY"), TEXT("가장 체력이 적은 몬스터를 공격하는 AI"), ESCTDTargetingAI::Greedy)));
	SeedParts.Add(BuildLegendaryPart(MakeControlPart(TEXT("part_control_potato"), TEXT("POTATO"), TEXT("가장 체력이 많은 몬스터를 공격하는 AI"), ESCTDTargetingAI::Potato)));
	SeedParts.Add(BuildLegendaryPart(MakeControlPart(TEXT("part_control_chaser"), TEXT("CHASER"), TEXT("가장 빠른 몬스터를 공격하는 AI"), ESCTDTargetingAI::Chaser)));
	SeedParts.Add(BuildLegendaryPart(MakeControlPart(TEXT("part_control_revenge"), TEXT("REVENGE"), TEXT("가장 공격력이 강한 몬스터를 공격하는 AI"), ESCTDTargetingAI::Revenge)));

	if (PartsRepository->GetOwnedPartCount() > 0 && !bShouldRerollSeedParts)
	{
		if (bHasLegacyMockParts)
		{
			UserRepository->Save();
		}
		return;
	}

	if (bShouldRerollSeedParts)
	{
		for (FSCTDOwnedTurretPartRecord& SeedPart : SeedParts)
		{
			if (const FSCTDOwnedTurretPartRecord* ExistingPart = SaveGame->OwnedParts.FindByPredicate([&SeedPart](const FSCTDOwnedTurretPartRecord& PartRecord)
			{
				return PartRecord.DefinitionId == SeedPart.DefinitionId;
			}))
			{
				SeedPart.InstanceId = ExistingPart->InstanceId;
			}
		}
		SaveGame->OwnedParts.RemoveAll(IsSeedPartDefinition);
	}

	if (PartsRepository->GetOwnedPartCount() == 0 || bShouldRerollSeedParts)
	{
		for (const FSCTDOwnedTurretPartRecord& SeedPart : SeedParts)
		{
			PartsRepository->AddPart(SeedPart);
		}
	}

	SaveGame->SaveVersion = CurrentMockSeedVersion;

	UserRepository->Save();
}
