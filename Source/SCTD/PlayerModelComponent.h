#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerModelComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = (SCTD), meta = (BlueprintSpawnableComponent))
class SCTD_API UPlayerModelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerModelComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Player Model")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Player Model")
	float GetCurrentFuel() const { return CurrentFuel; }

	UFUNCTION(BlueprintCallable, Category = "Player Model")
	void RecoverFuel(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Player Model")
	bool ConsumeBoostFuel(float DeltaSeconds);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxFuel = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Fuel", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FuelRecoveryRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MoveSpeed = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BoostSpeedMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float MovementMass = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SecondsToReachMaxSpeed = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Fuel", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BoostFuelConsumeRate = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Build", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BuildSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRange = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackDamage = 5.0f;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player Model|Runtime")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player Model|Runtime")
	float CurrentFuel = 100.0f;
};
