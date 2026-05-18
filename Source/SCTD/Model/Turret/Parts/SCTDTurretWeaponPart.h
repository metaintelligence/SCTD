#pragma once

#include "CoreMinimal.h"
#include "../../Combat/SCTDAttackTypes.h"
#include "SCTDTurretPart.h"
#include "SCTDTurretWeaponPart.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API USCTDTurretWeaponPart : public USCTDTurretPart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon")
	ESCTDTurretMountType MountType = ESCTDTurretMountType::Tower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinAttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxAttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRange = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AreaAttackRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon")
	bool bCanAreaAttack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CriticalChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float CriticalDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon")
	ESCTDAttackAttribute AttackAttribute = ESCTDAttackAttribute::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon")
	TArray<FSCTDStatusEffectChance> StatusEffectChances;
};
