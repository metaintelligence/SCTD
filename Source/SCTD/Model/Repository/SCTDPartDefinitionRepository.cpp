#include "SCTDPartDefinitionRepository.h"

#include "Engine/DataTable.h"

namespace
{
	const TCHAR* PartDefinitionContext = TEXT("SCTDPartDefinitionRepository");
	const TCHAR* PartOptionContext = TEXT("SCTDPartOptionRepository");
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
	if (!OptionTable || OptionId.IsNone())
	{
		return false;
	}

	if (const FSCTDTurretPartOptionDefinitionRow* FoundDefinition = OptionTable->FindRow<FSCTDTurretPartOptionDefinitionRow>(OptionId, PartOptionContext, false))
	{
		OutDefinition = *FoundDefinition;
		if (OutDefinition.OptionId.IsNone())
		{
			OutDefinition.OptionId = OptionId;
		}
		return true;
	}

	return false;
}

TArray<FSCTDTurretPartOptionDefinitionRow> USCTDPartDefinitionRepository::GetOptionsForPool(FName OptionPoolId, ESCTDTurretPartType PartType) const
{
	TArray<FSCTDTurretPartOptionDefinitionRow> Options;
	if (!OptionTable || OptionPoolId.IsNone())
	{
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
		FSCTDRolledTurretPartOption RolledOption;
		RolledOption.OptionId = SelectedOption.OptionId;
		RolledOption.Value = FMath::FRandRange(SelectedOption.MinValue, SelectedOption.MaxValue);
		RolledOptions.Add(RolledOption);
		CandidateOptions.RemoveAt(SelectedIndex);
	}

	return RolledOptions;
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
	FSCTDTurretPartDefinitionRow Definition;
	if (FindPartDefinition(OwnedPartRecord.DefinitionId, Definition))
	{
		ApplyDefinitionToOwnedPart(Definition, ResolvedRecord);
	}
	ApplyRolledOptionsToOwnedPart(OwnedPartRecord.RolledOptions, ResolvedRecord);
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

void USCTDPartDefinitionRepository::ApplyDefinitionToOwnedPart(const FSCTDTurretPartDefinitionRow& Definition, FSCTDOwnedTurretPartRecord& PartRecord) const
{
	PartRecord.DefinitionId = Definition.DefinitionId;
	PartRecord.PartType = Definition.PartType;
	PartRecord.DisplayName = Definition.DisplayName.ToString();
	PartRecord.BuildCost = Definition.BuildCost;
	PartRecord.BuildTimeSeconds = Definition.BuildTimeSeconds;
	PartRecord.BaseHealth = Definition.BaseHealth;
	PartRecord.Defense = Definition.Defense;
	PartRecord.MinAttackDamage = Definition.MinAttackDamage;
	PartRecord.MaxAttackDamage = Definition.MaxAttackDamage;
	PartRecord.AttackSpeed = Definition.AttackSpeed;
	PartRecord.AttackRange = Definition.AttackRange;
	PartRecord.AttackAttribute = Definition.AttackAttribute;
	PartRecord.StatusEffectChances = Definition.StatusEffectChances;
	PartRecord.AIProfileId = Definition.AIProfileId;
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

		if (OptionDefinition.TargetStat == TEXT("BuildCost"))
		{
			PartRecord.BuildCost += FMath::RoundToInt(RolledOption.Value);
		}
		else if (OptionDefinition.TargetStat == TEXT("BuildTimeSeconds"))
		{
			PartRecord.BuildTimeSeconds += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("BaseHealth"))
		{
			PartRecord.BaseHealth += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("Defense"))
		{
			PartRecord.Defense += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("AttackDamageRange"))
		{
			PartRecord.MinAttackDamage += RolledOption.Value;
			PartRecord.MaxAttackDamage += RolledOption.Value;
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
			PartRecord.AttackSpeed += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("AttackRange"))
		{
			PartRecord.AttackRange += RolledOption.Value;
		}
		else if (OptionDefinition.TargetStat == TEXT("StatusEffectChanceMultiplier"))
		{
			for (FSCTDStatusEffectChance& StatusEffectChance : PartRecord.StatusEffectChances)
			{
				if (StatusEffectChance.EffectType == OptionDefinition.TargetStatusEffectType)
				{
					StatusEffectChance.ChanceMultiplier *= RolledOption.Value;
				}
			}
		}
	}
}
