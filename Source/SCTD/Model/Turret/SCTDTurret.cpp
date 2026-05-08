#include "SCTDTurret.h"

#include "Parts/SCTDTurretBasePart.h"
#include "Parts/SCTDTurretControlPart.h"
#include "Parts/SCTDTurretWeaponPart.h"

bool USCTDTurret::IsComplete() const
{
	return BasePart && WeaponPart && ControlPart;
}

void USCTDTurret::SetParts(USCTDTurretBasePart* NewBasePart, USCTDTurretWeaponPart* NewWeaponPart, USCTDTurretControlPart* NewControlPart)
{
	BasePart = NewBasePart;
	WeaponPart = NewWeaponPart;
	ControlPart = NewControlPart;
}

int32 USCTDTurret::GetBuildCost() const
{
	int32 TotalCost = 0;
	TotalCost += BasePart ? BasePart->BuildCost : 0;
	TotalCost += WeaponPart ? WeaponPart->BuildCost : 0;
	TotalCost += ControlPart ? ControlPart->BuildCost : 0;
	return TotalCost;
}

float USCTDTurret::GetBuildTimeSeconds() const
{
	float TotalBuildTime = 0.0f;
	TotalBuildTime += BasePart ? BasePart->BuildTimeSeconds : 0.0f;
	TotalBuildTime += WeaponPart ? WeaponPart->BuildTimeSeconds : 0.0f;
	TotalBuildTime += ControlPart ? ControlPart->BuildTimeSeconds : 0.0f;
	return TotalBuildTime;
}

FSCTDTurretFinalStats USCTDTurret::GetFinalStats() const
{
	FSCTDTurretFinalStats FinalStats;
	FinalStats.BuildCost = GetBuildCost();
	FinalStats.BuildTimeSeconds = GetBuildTimeSeconds();

	if (BasePart)
	{
		FinalStats.MaxHealth = BasePart->BaseHealth;
		FinalStats.Defense = BasePart->Defense;
	}

	if (WeaponPart)
	{
		FinalStats.AttackDamage = WeaponPart->AttackDamage;
		FinalStats.AttackSpeed = WeaponPart->AttackSpeed;
	}

	if (ControlPart)
	{
		FinalStats.AIProfileId = ControlPart->AIProfileId;
	}

	return FinalStats;
}
