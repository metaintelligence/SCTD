#pragma once

#include "CoreMinimal.h"
#include "SCTDTurretPart.h"
#include "SCTDTurretWeaponPart.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API USCTDTurretWeaponPart : public USCTDTurretPart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Weapon", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRange = 4.0f;
};
