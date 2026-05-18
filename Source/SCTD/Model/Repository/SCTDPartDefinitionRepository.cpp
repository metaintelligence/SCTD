#include "SCTDPartDefinitionRepository.h"

#include "Engine/DataTable.h"

namespace
{
	const TCHAR* PartDefinitionContext = TEXT("SCTDPartDefinitionRepository");
	const TCHAR* PartOptionContext = TEXT("SCTDPartOptionRepository");
	const TCHAR* RarityContext = TEXT("SCTDItemRarityRepository");

	void AddPercentRanges(FSCTDTurretPartOptionDefinitionRow& Option, float CommonMin, float CommonMax, float AdvancedMin, float AdvancedMax, float RareMin, float RareMax, float LegendaryMin, float LegendaryMax)
	{
		Option.RarityRanges = {
			{ ESCTDItemRarity::Common, CommonMin, CommonMax },
			{ ESCTDItemRarity::Advanced, AdvancedMin, AdvancedMax },
			{ ESCTDItemRarity::Rare, RareMin, RareMax },
			{ ESCTDItemRarity::Legendary, LegendaryMin, LegendaryMax }
		};
	}

	FSCTDTurretPartOptionDefinitionRow MakeOption(FName OptionId, FName PoolId, ESCTDTurretPartType PartType, FName TargetStat, ESCTDPartOptionValueMode ValueMode)
	{
		FSCTDTurretPartOptionDefinitionRow Option;
		Option.OptionId = OptionId;
		Option.OptionPoolId = PoolId;
		Option.DisplayName = FText::FromName(OptionId);
		Option.AllowedPartType = PartType;
		Option.TargetStat = TargetStat;
		Option.ValueMode = ValueMode;
		Option.Weight = 1.0f;
		return Option;
	}
}

void USCTDPartDefinitionRepository::SetDefinitionTables(UDataTable* NewBasePartTable, UDataTable* NewWeaponPartTable, UDataTable* NewControlPartTable)
{
	BasePartTable = NewBasePartTable;
	WeaponPartTable = NewWeaponPartTable;
	ControlPartTable = NewControlPartTable;
}

void USCTDPartDefinitionRepository::SetOptionTable(UDataTable* NewOptionTable)
{
	OptionTable = NewOptionTable;
}

void USCTDPartDefinitionRepository::SetRarityTable(UDataTable* NewRarityTable)
{
	RarityTable = NewRarityTable;
}

bool USCTDPartDefinitionRepository::FindPartDefinition(FName DefinitionId, FSCTDTurretPartDefinitionRow& OutDefinition) const
{
	if (DefinitionId.IsNone())
	{
		return false;
	}

	for (const ESCTDTurretPartType PartType : { ESCTDTurretPartType::Base, ESCTDTurretPartType::Weapon, ESCTDTurretPartType::Control })
	{
		if (FindPartDefinitionByType(PartType, DefinitionId, OutDefinition))
		{
			return true;
		}
	}

	return false;
}

bool USCTDPartDefinitionRepository::FindPartDefinitionByType(ESCTDTurretPartType PartType, FName DefinitionId, FSCTDTurretPartDefinitionRow& OutDefinition) const
{
	const UDataTable* PartTable = GetPartTable(PartType);
	if (!PartTable || DefinitionId.IsNone())
	{
		return false;
	}

	const FSCTDTurretPartDefinitionRow* FoundDefinition = PartTable->FindRow<FSCTDTurretPartDefinitionRow>(DefinitionId, PartDefinitionContext, false);
	if (!FoundDefinition)
	{
		return false;
	}

	OutDefinition = *FoundDefinition;
	if (OutDefinition.DefinitionId.IsNone())
	{
		OutDefinition.DefinitionId = DefinitionId;
	}
	OutDefinition.PartType = PartType;
	return true;
}

TArray<FSCTDTurretPartDefinitionRow> USCTDPartDefinitionRepository::GetPartDefinitionsByType(ESCTDTurretPartType PartType) const
{
	TArray<FSCTDTurretPartDefinitionRow> Definitions;
	const UDataTable* PartTable = GetPartTable(PartType);
	if (!PartTable)
	{
		return Definitions;
	}

	for (const FName RowName : PartTable->GetRowNames())
	{
		if (const FSCTDTurretPartDefinitionRow* Definition = PartTable->FindRow<FSCTDTurretPartDefinitionRow>(RowName, PartDefinitionContext, false))
		{
			FSCTDTurretPartDefinitionRow DefinitionCopy = *Definition;
			if (DefinitionCopy.DefinitionId.IsNone())
			{
				DefinitionCopy.DefinitionId = RowName;
			}
			DefinitionCopy.PartType = PartType;
			Definitions.Add(DefinitionCopy);
		}
	}

	return Definitions;
}

bool USCTDPartDefinitionRepository::FindOptionDefinition(FName OptionId, FSCTDTurretPartOptionDefinitionRow& OutDefinition) const
{
	if (OptionId.IsNone())
	{
		return false;
	}

	if (OptionTable)
	{
		if (const FSCTDTurretPartOptionDefinitionRow* FoundDefinition = OptionTable->FindRow<FSCTDTurretPartOptionDefinitionRow>(OptionId, PartOptionContext, false))
		{
			OutDefinition = *FoundDefinition;
			if (OutDefinition.OptionId.IsNone())
			{
				OutDefinition.OptionId = OptionId;
			}
			return true;
		}
	}

	for (const TPair<FName, ESCTDTurretPartType> PoolAndType : {
		TPair<FName, ESCTDTurretPartType>(TEXT("BaseWeaponPartOptionTable"), ESCTDTurretPartType::Weapon),
		TPair<FName, ESCTDTurretPartType>(TEXT("BaseBodyPartOptionTable"), ESCTDTurretPartType::Base),
		TPair<FName, ESCTDTurretPartType>(TEXT("BaseControlPartOptionTable"), ESCTDTurretPartType::Control)
	})
	{
		for (const FSCTDTurretPartOptionDefinitionRow& OptionDefinition : GetOptionsForPool(PoolAndType.Key, PoolAndType.Value))
		{
			if (OptionDefinition.OptionId == OptionId)
			{
				OutDefinition = OptionDefinition;
				return true;
			}
		}
	}

	return false;
}

TArray<FSCTDTurretPartOptionDefinitionRow> USCTDPartDefinitionRepository::GetOptionsForPool(FName OptionPoolId, ESCTDTurretPartType PartType) const
{
	TArray<FSCTDTurretPartOptionDefinitionRow> Options;
	if (!OptionTable || OptionPoolId.IsNone())
	{
		if (!OptionPoolId.IsNone())
		{
			auto AddDefault = [&Options, OptionPoolId, PartType](FName OptionId, FName TargetStat, ESCTDPartOptionValueMode ValueMode, float CommonMin, float CommonMax, float AdvancedMin, float AdvancedMax, float RareMin, float RareMax, float LegendaryMin, float LegendaryMax)
			{
				FSCTDTurretPartOptionDefinitionRow Option = MakeOption(OptionId, OptionPoolId, PartType, TargetStat, ValueMode);
				AddPercentRanges(Option, CommonMin, CommonMax, AdvancedMin, AdvancedMax, RareMin, RareMax, LegendaryMin, LegendaryMax);
				Options.Add(Option);
			};

			if (OptionPoolId == TEXT("BaseWeaponPartOptionTable"))
			{
				AddDefault(TEXT("IncreaseAttackRange"), TEXT("AttackRange"), ESCTDPartOptionValueMode::AddFlat, 1, 1, 1, 1, 2, 2, 2, 2);
				AddDefault(TEXT("PhysicalDamageBonus"), TEXT("PhysicalDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("FireDamageBonus"), TEXT("FireDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("LightningDamageBonus"), TEXT("LightningDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("FrostDamageBonus"), TEXT("FrostDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseAttackSpeed"), TEXT("AttackSpeed"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 20, 25, 20, 30);
				AddDefault(TEXT("IncreaseAreaRange"), TEXT("AreaAttackRange"), ESCTDPartOptionValueMode::AddFlat, 1, 1, 1, 1, 2, 2, 2, 2);
				AddDefault(TEXT("IncreaseCriticalChance"), TEXT("CriticalChance"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("IncreaseCriticalDamage"), TEXT("CriticalDamageMultiplier"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ReduceBuildCost"), TEXT("BuildCostReduction"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ReduceBuildTime"), TEXT("BuildTimeReduction"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ExtraScrapGain"), TEXT("ScrapGainBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
			}
			else if (OptionPoolId == TEXT("BaseBodyPartOptionTable"))
			{
				AddDefault(TEXT("IncreaseHealth"), TEXT("BaseHealth"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseDefense"), TEXT("Defense"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseSelfRepair"), TEXT("SelfRepairPerSecond"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseAttackSpeed"), TEXT("AttackSpeed"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 20, 25, 20, 30);
				AddDefault(TEXT("IncreaseCriticalChance"), TEXT("CriticalChance"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("IncreaseCriticalDamage"), TEXT("CriticalDamageMultiplier"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ReduceBuildCost"), TEXT("BuildCostReduction"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ReduceBuildTime"), TEXT("BuildTimeReduction"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ExtraScrapGain"), TEXT("ScrapGainBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
			}
			else if (OptionPoolId == TEXT("BaseControlPartOptionTable"))
			{
				AddDefault(TEXT("PhysicalDamageBonus"), TEXT("PhysicalDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("FireDamageBonus"), TEXT("FireDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("LightningDamageBonus"), TEXT("LightningDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("FrostDamageBonus"), TEXT("FrostDamageBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseHealth"), TEXT("BaseHealth"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseDefense"), TEXT("Defense"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseSelfRepair"), TEXT("SelfRepairPerSecond"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("IncreaseCriticalChance"), TEXT("CriticalChance"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("IncreaseCriticalDamage"), TEXT("CriticalDamageMultiplier"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("AmplifyStatusChance"), TEXT("StatusEffectChanceMultiplier"), ESCTDPartOptionValueMode::AddPercentOfBase, 10, 20, 10, 25, 15, 25, 20, 30);
				AddDefault(TEXT("ReduceBuildCost"), TEXT("BuildCostReduction"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ReduceBuildTime"), TEXT("BuildTimeReduction"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);
				AddDefault(TEXT("ExtraScrapGain"), TEXT("ScrapGainBonusRatio"), ESCTDPartOptionValueMode::AddPercentOfBase, 5, 10, 5, 10, 5, 15, 10, 20);

				auto AddStatusOption = [&Options, OptionPoolId, PartType](FName OptionId, ESCTDStatusEffectType EffectType, float MinDuration, float MaxDuration, float MinValue, float MaxValue)
				{
					FSCTDTurretPartOptionDefinitionRow Option = MakeOption(OptionId, OptionPoolId, PartType, TEXT("StatusEffectSpec"), ESCTDPartOptionValueMode::AddFlat);
					Option.TargetStatusEffectType = EffectType;
					Option.StatusEffectSpec.EffectType = EffectType;
					Option.StatusEffectSpec.MinDurationSeconds = MinDuration;
					Option.StatusEffectSpec.MaxDurationSeconds = MaxDuration;
					Option.StatusEffectSpec.MinValue = MinValue;
					Option.StatusEffectSpec.MaxValue = MaxValue;
					Options.Add(Option);
				};
				AddStatusOption(TEXT("PhysicalDestruction"), ESCTDStatusEffectType::Destruction, 5.0f, 5.0f, 0.30f, 0.70f);
				AddStatusOption(TEXT("PhysicalConcussion"), ESCTDStatusEffectType::Concussion, 3.0f, 6.0f, 0.0f, 0.0f);
				AddStatusOption(TEXT("FireIgnite"), ESCTDStatusEffectType::Ignite, 3.0f, 6.0f, 0.03f, 0.03f);
				AddStatusOption(TEXT("FireTileBurn"), ESCTDStatusEffectType::Fire, 3.0f, 6.0f, 0.03f, 0.03f);
				AddStatusOption(TEXT("LightningStagger"), ESCTDStatusEffectType::Stagger, 0.2f, 0.5f, 0.0f, 0.0f);
				AddStatusOption(TEXT("LightningExecute"), ESCTDStatusEffectType::Execute, 3.0f, 6.0f, 0.10f, 0.10f);
				AddStatusOption(TEXT("FrostChill"), ESCTDStatusEffectType::Chill, 5.0f, 5.0f, 0.30f, 0.70f);
				AddStatusOption(TEXT("FrostFreeze"), ESCTDStatusEffectType::Freeze, 3.0f, 6.0f, 0.0f, 0.0f);
			}
		}
		return Options;
	}

	for (const FName RowName : OptionTable->GetRowNames())
	{
		const FSCTDTurretPartOptionDefinitionRow* Option = OptionTable->FindRow<FSCTDTurretPartOptionDefinitionRow>(RowName, PartOptionContext, false);
		if (!Option || Option->OptionPoolId != OptionPoolId || Option->AllowedPartType != PartType || Option->Weight <= 0.0f)
		{
			continue;
		}

		FSCTDTurretPartOptionDefinitionRow OptionCopy = *Option;
		if (OptionCopy.OptionId.IsNone())
		{
			OptionCopy.OptionId = RowName;
		}
		Options.Add(OptionCopy);
	}

	return Options;
}

TArray<FSCTDRolledTurretPartOption> USCTDPartDefinitionRepository::RollOptions(FName OptionPoolId, ESCTDTurretPartType PartType, int32 OptionCount) const
{
	return RollOptionsForRarity(OptionPoolId, PartType, ESCTDItemRarity::Common, OptionCount);
}

TArray<FSCTDRolledTurretPartOption> USCTDPartDefinitionRepository::RollOptionsForRarity(FName OptionPoolId, ESCTDTurretPartType PartType, ESCTDItemRarity Rarity, int32 OptionCount) const
{
	TArray<FSCTDRolledTurretPartOption> RolledOptions;
	TArray<FSCTDTurretPartOptionDefinitionRow> CandidateOptions = GetOptionsForPool(OptionPoolId, PartType);
	OptionCount = FMath::Clamp(OptionCount, 0, CandidateOptions.Num());

	for (int32 OptionIndex = 0; OptionIndex < OptionCount; ++OptionIndex)
	{
		float TotalWeight = 0.0f;
		for (const FSCTDTurretPartOptionDefinitionRow& CandidateOption : CandidateOptions)
		{
			TotalWeight += FMath::Max(0.0f, CandidateOption.Weight);
		}

		if (TotalWeight <= KINDA_SMALL_NUMBER)
		{
			break;
		}

		float Roll = FMath::FRandRange(0.0f, TotalWeight);
		int32 SelectedIndex = INDEX_NONE;
		for (int32 CandidateIndex = 0; CandidateIndex < CandidateOptions.Num(); ++CandidateIndex)
		{
			Roll -= FMath::Max(0.0f, CandidateOptions[CandidateIndex].Weight);
			if (Roll <= 0.0f)
			{
				SelectedIndex = CandidateIndex;
				break;
			}
		}

		if (SelectedIndex == INDEX_NONE)
		{
			SelectedIndex = CandidateOptions.Num() - 1;
		}

		const FSCTDTurretPartOptionDefinitionRow SelectedOption = CandidateOptions[SelectedIndex];
		float MinValue = SelectedOption.MinValue;
		float MaxValue = SelectedOption.MaxValue;
		TryGetOptionValueRangeForRarity(SelectedOption, Rarity, MinValue, MaxValue);
		FSCTDRolledTurretPartOption RolledOption;
		RolledOption.OptionId = SelectedOption.OptionId;
		RolledOption.Value = FMath::FRandRange(MinValue, MaxValue);
		RolledOptions.Add(RolledOption);
		CandidateOptions.RemoveAt(SelectedIndex);
	}

	return RolledOptions;
}

TArray<FSCTDRolledTurretPartOption> USCTDPartDefinitionRepository::RollOptionsForDefinition(const FSCTDTurretPartDefinitionRow& Definition, ESCTDItemRarity Rarity, int32 OptionCount) const
{
	TArray<FSCTDTurretPartOptionDefinitionRow> CandidateOptions = GetOptionsForPool(Definition.OptionPoolId, Definition.PartType);
	if (Definition.PartType == ESCTDTurretPartType::Weapon && !Definition.bCanAreaAttack)
	{
		CandidateOptions.RemoveAll([](const FSCTDTurretPartOptionDefinitionRow& OptionDefinition)
		{
			return OptionDefinition.OptionId == TEXT("IncreaseAreaRange")
				|| OptionDefinition.TargetStat == TEXT("AreaAttackRange");
		});
	}

	TArray<FSCTDRolledTurretPartOption> RolledOptions;
	OptionCount = FMath::Clamp(OptionCount, 0, CandidateOptions.Num());
	for (int32 OptionIndex = 0; OptionIndex < OptionCount; ++OptionIndex)
	{
		float TotalWeight = 0.0f;
		for (const FSCTDTurretPartOptionDefinitionRow& CandidateOption : CandidateOptions)
		{
			TotalWeight += FMath::Max(0.0f, CandidateOption.Weight);
		}

		if (TotalWeight <= KINDA_SMALL_NUMBER)
		{
			break;
		}

		float Roll = FMath::FRandRange(0.0f, TotalWeight);
		int32 SelectedIndex = INDEX_NONE;
		for (int32 CandidateIndex = 0; CandidateIndex < CandidateOptions.Num(); ++CandidateIndex)
		{
			Roll -= FMath::Max(0.0f, CandidateOptions[CandidateIndex].Weight);
			if (Roll <= 0.0f)
			{
				SelectedIndex = CandidateIndex;
				break;
			}
		}

		if (SelectedIndex == INDEX_NONE)
		{
			SelectedIndex = CandidateOptions.Num() - 1;
		}

		const FSCTDTurretPartOptionDefinitionRow SelectedOption = CandidateOptions[SelectedIndex];
		float MinValue = SelectedOption.MinValue;
		float MaxValue = SelectedOption.MaxValue;
		TryGetOptionValueRangeForRarity(SelectedOption, Rarity, MinValue, MaxValue);
		FSCTDRolledTurretPartOption RolledOption;
		RolledOption.OptionId = SelectedOption.OptionId;
		RolledOption.Value = FMath::FRandRange(MinValue, MaxValue);
		RolledOptions.Add(RolledOption);
		CandidateOptions.RemoveAt(SelectedIndex);
	}

	return RolledOptions;
}

FSCTDItemRarityDefinitionRow USCTDPartDefinitionRepository::RollRarity() const
{
	TArray<FSCTDItemRarityDefinitionRow> CandidateRarities;
	if (RarityTable)
	{
		for (const FName RowName : RarityTable->GetRowNames())
		{
			if (const FSCTDItemRarityDefinitionRow* RarityDefinition = RarityTable->FindRow<FSCTDItemRarityDefinitionRow>(RowName, RarityContext, false))
			{
				if (RarityDefinition->Weight > 0.0f)
				{
					CandidateRarities.Add(*RarityDefinition);
				}
			}
		}
	}

	if (CandidateRarities.Num() == 0)
	{
		CandidateRarities = {
			GetDefaultRarityDefinition(ESCTDItemRarity::Common),
			GetDefaultRarityDefinition(ESCTDItemRarity::Advanced),
			GetDefaultRarityDefinition(ESCTDItemRarity::Rare),
			GetDefaultRarityDefinition(ESCTDItemRarity::Legendary),
			GetDefaultRarityDefinition(ESCTDItemRarity::NoDrop)
		};
	}

	float TotalWeight = 0.0f;
	for (const FSCTDItemRarityDefinitionRow& RarityDefinition : CandidateRarities)
	{
		TotalWeight += FMath::Max(0.0f, RarityDefinition.Weight);
	}

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	for (const FSCTDItemRarityDefinitionRow& RarityDefinition : CandidateRarities)
	{
		Roll -= FMath::Max(0.0f, RarityDefinition.Weight);
		if (Roll <= 0.0f)
		{
			return RarityDefinition;
		}
	}

	return CandidateRarities.Last();
}

bool USCTDPartDefinitionRepository::BuildOwnedPartFromDefinition(FName DefinitionId, const TArray<FSCTDRolledTurretPartOption>& RolledOptions, FSCTDOwnedTurretPartRecord& OutPartRecord) const
{
	FSCTDTurretPartDefinitionRow Definition;
	if (!FindPartDefinition(DefinitionId, Definition))
	{
		return false;
	}

	OutPartRecord = FSCTDOwnedTurretPartRecord();
	OutPartRecord.InstanceId = FGuid::NewGuid();
	OutPartRecord.DefinitionId = Definition.DefinitionId.IsNone() ? DefinitionId : Definition.DefinitionId;
	OutPartRecord.RolledOptions = RolledOptions;
	ApplyDefinitionToOwnedPart(Definition, OutPartRecord);
	ApplyRolledOptionsToOwnedPart(RolledOptions, OutPartRecord);
	return true;
}

FSCTDOwnedTurretPartRecord USCTDPartDefinitionRepository::ResolveOwnedPart(const FSCTDOwnedTurretPartRecord& OwnedPartRecord) const
{
	FSCTDOwnedTurretPartRecord ResolvedRecord = OwnedPartRecord;
	bool bResolvedFromDefinition = false;
	FSCTDTurretPartDefinitionRow Definition;
	if (FindPartDefinition(OwnedPartRecord.DefinitionId, Definition))
	{
		ApplyDefinitionToOwnedPart(Definition, ResolvedRecord);
		bResolvedFromDefinition = true;
	}

	if (bResolvedFromDefinition || OwnedPartRecord.AdditionalOptions.Num() == 0)
	{
		ApplyRolledOptionsToOwnedPart(OwnedPartRecord.RolledOptions, ResolvedRecord);
	}
	return ResolvedRecord;
}

const UDataTable* USCTDPartDefinitionRepository::GetPartTable(ESCTDTurretPartType PartType) const
{
	switch (PartType)
	{
	case ESCTDTurretPartType::Base:
		return BasePartTable;
	case ESCTDTurretPartType::Weapon:
		return WeaponPartTable;
	case ESCTDTurretPartType::Control:
		return ControlPartTable;
	default:
		return nullptr;
	}
}

FSCTDItemRarityDefinitionRow USCTDPartDefinitionRepository::GetDefaultRarityDefinition(ESCTDItemRarity Rarity) const
{
	FSCTDItemRarityDefinitionRow Definition;
	Definition.Rarity = Rarity;
	switch (Rarity)
	{
	case ESCTDItemRarity::Common:
		Definition.Weight = 50.0f;
		Definition.DisplayColor = FLinearColor::White;
		Definition.MinOptionCount = 1;
		Definition.MaxOptionCount = 1;
		break;
	case ESCTDItemRarity::Advanced:
		Definition.Weight = 25.0f;
		Definition.DisplayColor = FLinearColor(0.60f, 1.0f, 0.60f, 1.0f);
		Definition.MinOptionCount = 2;
		Definition.MaxOptionCount = 2;
		break;
	case ESCTDItemRarity::Rare:
		Definition.Weight = 12.5f;
		Definition.DisplayColor = FLinearColor(0.50f, 0.82f, 1.0f, 1.0f);
		Definition.MinOptionCount = 3;
		Definition.MaxOptionCount = 3;
		break;
	case ESCTDItemRarity::Legendary:
		Definition.Weight = 6.25f;
		Definition.DisplayColor = FLinearColor(0.84f, 0.62f, 1.0f, 1.0f);
		Definition.MinOptionCount = 4;
		Definition.MaxOptionCount = 5;
		break;
	case ESCTDItemRarity::NoDrop:
	default:
		Definition.Weight = 6.25f;
		Definition.DisplayColor = FLinearColor::Transparent;
		Definition.MinOptionCount = 0;
		Definition.MaxOptionCount = 0;
		break;
	}
	return Definition;
}

bool USCTDPartDefinitionRepository::TryGetOptionValueRangeForRarity(const FSCTDTurretPartOptionDefinitionRow& OptionDefinition, ESCTDItemRarity Rarity, float& OutMinValue, float& OutMaxValue) const
{
	for (const FSCTDPartOptionRarityRange& RarityRange : OptionDefinition.RarityRanges)
	{
		if (RarityRange.Rarity == Rarity)
		{
			OutMinValue = RarityRange.MinValue;
			OutMaxValue = RarityRange.MaxValue;
			return true;
		}
	}
	return false;
}

void USCTDPartDefinitionRepository::ApplyDefinitionToOwnedPart(const FSCTDTurretPartDefinitionRow& Definition, FSCTDOwnedTurretPartRecord& PartRecord) const
{
	PartRecord.DefinitionId = Definition.DefinitionId;
	PartRecord.PartType = Definition.PartType;
	PartRecord.DisplayName = Definition.DisplayName.ToString();
	PartRecord.Description = Definition.Description.ToString();
	PartRecord.BuildCost = Definition.BuildCost;
	PartRecord.BuildTimeSeconds = Definition.BuildTimeSeconds;
	PartRecord.MountType = Definition.MountType;
	PartRecord.BaseHealth = Definition.BaseHealth;
	PartRecord.Defense = Definition.Defense;
	PartRecord.SelfRepairPerSecond = Definition.SelfRepairPerSecond;
	PartRecord.MinAttackDamage = Definition.MinAttackDamage;
	PartRecord.MaxAttackDamage = Definition.MaxAttackDamage;
		PartRecord.AttackSpeed = Definition.AttackSpeed;
		PartRecord.AttackRange = Definition.AttackRange;
	PartRecord.bCanAreaAttack = Definition.bCanAreaAttack;
	PartRecord.AreaAttackRange = Definition.bCanAreaAttack ? Definition.AreaAttackRange : 0.0f;
		PartRecord.CriticalChance = Definition.CriticalChance;
	PartRecord.CriticalDamageMultiplier = Definition.CriticalDamageMultiplier;
	PartRecord.AttackAttribute = Definition.AttackAttribute;
	PartRecord.StatusEffectChances = Definition.StatusEffectChances;
	PartRecord.StatusEffectSpecs = Definition.StatusEffectSpecs;
	PartRecord.PhysicalDamageBonusRatio = 0.0f;
	PartRecord.FireDamageBonusRatio = 0.0f;
	PartRecord.LightningDamageBonusRatio = 0.0f;
	PartRecord.FrostDamageBonusRatio = 0.0f;
	PartRecord.ScrapGainBonusRatio = 0.0f;
	PartRecord.TargetingAI = Definition.TargetingAI;
	PartRecord.AIProfileId = StaticEnum<ESCTDTargetingAI>()->GetNameByValue(static_cast<int64>(Definition.TargetingAI));
	PartRecord.AdditionalOptions.Reset();
}

void USCTDPartDefinitionRepository::ApplyRolledOptionsToOwnedPart(const TArray<FSCTDRolledTurretPartOption>& RolledOptions, FSCTDOwnedTurretPartRecord& PartRecord) const
{
	for (const FSCTDRolledTurretPartOption& RolledOption : RolledOptions)
	{
		FSCTDTurretPartOptionDefinitionRow OptionDefinition;
		if (!FindOptionDefinition(RolledOption.OptionId, OptionDefinition))
		{
			continue;
		}

		FSCTDTurretPartOption LegacyOption;
		LegacyOption.OptionId = RolledOption.OptionId;
		LegacyOption.DisplayName = OptionDefinition.DisplayName;
		LegacyOption.Value = RolledOption.Value;
		PartRecord.AdditionalOptions.Add(LegacyOption);

		float EffectiveValue = RolledOption.Value;
		if (OptionDefinition.ValueMode == ESCTDPartOptionValueMode::AddPercentOfBase)
		{
			EffectiveValue *= 0.01f;
		}

		if (OptionDefinition.TargetStat == TEXT("BuildCost"))
		{
			PartRecord.BuildCost += FMath::RoundToInt(RolledOption.Value);
		}
		else if (OptionDefinition.TargetStat == TEXT("BuildCostReduction"))
		{
			PartRecord.BuildCost = FMath::Max(0, PartRecord.BuildCost - FMath::CeilToInt(static_cast<float>(PartRecord.BuildCost) * EffectiveValue));
		}
		else if (OptionDefinition.TargetStat == TEXT("BuildTimeSeconds"))
		{
			PartRecord.BuildTimeSeconds += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("BuildTimeReduction"))
		{
			PartRecord.BuildTimeSeconds = FMath::Max(0.1f, PartRecord.BuildTimeSeconds * (1.0f - EffectiveValue));
		}
		else if (OptionDefinition.TargetStat == TEXT("BaseHealth"))
		{
			PartRecord.BaseHealth += OptionDefinition.ValueMode == ESCTDPartOptionValueMode::AddPercentOfBase
				? (PartRecord.PartType == ESCTDTurretPartType::Base ? PartRecord.BaseHealth * EffectiveValue : EffectiveValue)
				: RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("Defense"))
		{
			PartRecord.Defense += OptionDefinition.ValueMode == ESCTDPartOptionValueMode::AddPercentOfBase
				? (PartRecord.PartType == ESCTDTurretPartType::Base ? PartRecord.Defense * EffectiveValue : EffectiveValue)
				: RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("SelfRepairPerSecond"))
		{
			PartRecord.SelfRepairPerSecond += OptionDefinition.ValueMode == ESCTDPartOptionValueMode::AddPercentOfBase
				? (PartRecord.PartType == ESCTDTurretPartType::Base ? PartRecord.SelfRepairPerSecond * EffectiveValue : EffectiveValue)
				: RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("AttackDamageRange"))
		{
			const float DamageDelta = OptionDefinition.ValueMode == ESCTDPartOptionValueMode::AddPercentOfBase ? PartRecord.MaxAttackDamage * EffectiveValue : RolledOption.Value;
			PartRecord.MinAttackDamage += DamageDelta;
			PartRecord.MaxAttackDamage += DamageDelta;
		}
		else if (OptionDefinition.TargetStat == TEXT("MinAttackDamage"))
		{
			PartRecord.MinAttackDamage += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("MaxAttackDamage"))
		{
			PartRecord.MaxAttackDamage += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("AttackSpeed"))
		{
			PartRecord.AttackSpeed += OptionDefinition.ValueMode == ESCTDPartOptionValueMode::AddPercentOfBase
				? (PartRecord.PartType == ESCTDTurretPartType::Weapon ? PartRecord.AttackSpeed * EffectiveValue : EffectiveValue)
				: RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("AttackRange"))
		{
			PartRecord.AttackRange += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("AreaAttackRange"))
		{
			PartRecord.AreaAttackRange += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("CriticalChance"))
		{
			PartRecord.CriticalChance = FMath::Clamp(PartRecord.CriticalChance + EffectiveValue, 0.0f, 1.0f);
		}
		else if (OptionDefinition.TargetStat == TEXT("CriticalDamageMultiplier"))
		{
			PartRecord.CriticalDamageMultiplier = FMath::Max(1.0f, PartRecord.CriticalDamageMultiplier + EffectiveValue);
		}
		else if (OptionDefinition.TargetStat == TEXT("PhysicalDamageBonusRatio"))
		{
			PartRecord.PhysicalDamageBonusRatio += EffectiveValue;
		}
		else if (OptionDefinition.TargetStat == TEXT("FireDamageBonusRatio"))
		{
			PartRecord.FireDamageBonusRatio += EffectiveValue;
		}
		else if (OptionDefinition.TargetStat == TEXT("LightningDamageBonusRatio"))
		{
			PartRecord.LightningDamageBonusRatio += EffectiveValue;
		}
		else if (OptionDefinition.TargetStat == TEXT("FrostDamageBonusRatio"))
		{
			PartRecord.FrostDamageBonusRatio += EffectiveValue;
		}
		else if (OptionDefinition.TargetStat == TEXT("ScrapGainBonusRatio"))
		{
			PartRecord.ScrapGainBonusRatio += EffectiveValue;
		}
		else if (OptionDefinition.TargetStat == TEXT("StatusEffectChanceMultiplier"))
		{
			for (FSCTDStatusEffectChance& StatusEffectChance : PartRecord.StatusEffectChances)
			{
				if (OptionDefinition.TargetStatusEffectType == ESCTDStatusEffectType::None
					|| StatusEffectChance.EffectType == OptionDefinition.TargetStatusEffectType)
				{
					StatusEffectChance.ChanceMultiplier *= 1.0f + EffectiveValue;
				}
			}
		}
		else if (OptionDefinition.TargetStat == TEXT("StatusEffectSpec"))
		{
			PartRecord.StatusEffectSpecs.Add(OptionDefinition.StatusEffectSpec);
		}
	}
}
