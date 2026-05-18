#include "SCTDPartDefinitionRepository.h"

#include "Engine/DataTable.h"

namespace
{
	const TCHAR* PartDefinitionContext = TEXT("SCTDPartDefinitionRepository");
	const TCHAR* PartOptionContext = TEXT("SCTDPartOptionRepository");
	const TCHAR* RarityContext = TEXT("SCTDItemRarityRepository");
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

		for (const FName RowName : OptionTable->GetRowNames())
		{
			if (const FSCTDTurretPartOptionDefinitionRow* FoundDefinition = OptionTable->FindRow<FSCTDTurretPartOptionDefinitionRow>(RowName, PartOptionContext, false))
			{
				if (FoundDefinition->OptionId == OptionId)
				{
					OutDefinition = *FoundDefinition;
					return true;
				}
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
		UE_LOG(LogTemp, Error, TEXT("Part option table is not configured for option pool '%s'."), *OptionPoolId.ToString());
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
		UE_LOG(LogTemp, Error, TEXT("Item rarity table is not configured or has no weighted rows. Item drop will be skipped."));
		FSCTDItemRarityDefinitionRow NoDropDefinition;
		NoDropDefinition.Rarity = ESCTDItemRarity::NoDrop;
		NoDropDefinition.Weight = 0.0f;
		NoDropDefinition.DisplayColor = FLinearColor::Transparent;
		NoDropDefinition.MinOptionCount = 0;
		NoDropDefinition.MaxOptionCount = 0;
		return NoDropDefinition;
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
	switch (Rarity)
	{
	case ESCTDItemRarity::Common:
		OutMinValue = OptionDefinition.CommonMinValue;
		OutMaxValue = OptionDefinition.CommonMaxValue;
		return true;
	case ESCTDItemRarity::Advanced:
		OutMinValue = OptionDefinition.AdvancedMinValue;
		OutMaxValue = OptionDefinition.AdvancedMaxValue;
		return true;
	case ESCTDItemRarity::Rare:
		OutMinValue = OptionDefinition.RareMinValue;
		OutMaxValue = OptionDefinition.RareMaxValue;
		return true;
	case ESCTDItemRarity::Legendary:
		OutMinValue = OptionDefinition.LegendaryMinValue;
		OutMaxValue = OptionDefinition.LegendaryMaxValue;
		return true;
	default:
		return false;
	}
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
		const FName OptionPoolId = PartRecord.PartType == ESCTDTurretPartType::Weapon
			? FName(TEXT("BaseWeaponPartOptionTable"))
			: PartRecord.PartType == ESCTDTurretPartType::Control
				? FName(TEXT("BaseControlPartOptionTable"))
				: FName(TEXT("BaseBodyPartOptionTable"));
		const TArray<FSCTDTurretPartOptionDefinitionRow> PoolOptions = GetOptionsForPool(OptionPoolId, PartRecord.PartType);
		const FSCTDTurretPartOptionDefinitionRow* FoundOptionDefinition = PoolOptions.FindByPredicate(
			[&RolledOption](const FSCTDTurretPartOptionDefinitionRow& Candidate)
			{
				return Candidate.OptionId == RolledOption.OptionId;
			});
		if (!FoundOptionDefinition)
		{
			continue;
		}
		OptionDefinition = *FoundOptionDefinition;

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
			FSCTDStatusEffectSpec StatusEffectSpec = OptionDefinition.StatusEffectSpec;
			if (StatusEffectSpec.EffectType == ESCTDStatusEffectType::None)
			{
				StatusEffectSpec.EffectType = OptionDefinition.TargetStatusEffectType;
				StatusEffectSpec.MinDurationSeconds = OptionDefinition.StatusMinDurationSeconds;
				StatusEffectSpec.MaxDurationSeconds = OptionDefinition.StatusMaxDurationSeconds;
				StatusEffectSpec.MinValue = OptionDefinition.StatusMinValue;
				StatusEffectSpec.MaxValue = OptionDefinition.StatusMaxValue;
			}
			PartRecord.StatusEffectSpecs.Add(StatusEffectSpec);
		}
	}
}
