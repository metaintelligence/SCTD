#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SCTDTurret.generated.h"

class USCTDTurretBasePart;
class USCTDTurretControlPart;
class USCTDTurretWeaponPart;

USTRUCT(BlueprintType)
struct FSCTDTurretFinalStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Turret|Stats")
	int32 BuildCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Turret|Stats")
	float BuildTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Turret|Stats")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Turret|Stats")
	float Defense = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Turret|Stats")
	float AttackDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Turret|Stats")
	float AttackSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Turret|Stats")
	FName AIProfileId = NAME_None;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API USCTDTurret : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Turret")
	bool IsComplete() const;

	UFUNCTION(BlueprintCallable, Category = "Turret")
	void SetParts(USCTDTurretBasePart* NewBasePart, USCTDTurretWeaponPart* NewWeaponPart, USCTDTurretControlPart* NewControlPart);

	UFUNCTION(BlueprintPure, Category = "Turret|Stats")
	int32 GetBuildCost() const;

	UFUNCTION(BlueprintPure, Category = "Turret|Stats")
	float GetBuildTimeSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Turret|Stats")
	FSCTDTurretFinalStats GetFinalStats() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Turret|Parts")
	TObjectPtr<USCTDTurretBasePart> BasePart;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Turret|Parts")
	TObjectPtr<USCTDTurretWeaponPart> WeaponPart;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Turret|Parts")
	TObjectPtr<USCTDTurretControlPart> ControlPart;
};
