#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatusValueChangedSignature, float, CurrentValue, float, MaxValue);

UCLASS(Blueprintable, ClassGroup = (SCTD), meta = (BlueprintSpawnableComponent))
class SCTD_API UStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Status|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Status|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Status|Health")
	float GetHealthRatio() const;

	UFUNCTION(BlueprintCallable, Category = "Status|Health")
	void SetHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Status|Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "Status|Boost")
	bool UsesBoost() const { return bUsesBoost; }

	UFUNCTION(BlueprintPure, Category = "Status|Boost")
	float GetCurrentBoost() const { return CurrentBoost; }

	UFUNCTION(BlueprintPure, Category = "Status|Boost")
	float GetMaxBoost() const { return MaxBoost; }

	UFUNCTION(BlueprintPure, Category = "Status|Boost")
	float GetBoostRatio() const;

	UFUNCTION(BlueprintCallable, Category = "Status|Boost")
	void SetBoost(float NewBoost);

	UFUNCTION(BlueprintCallable, Category = "Status|Boost")
	void RecoverBoost(float DeltaSeconds, float RecoveryRate);

	UFUNCTION(BlueprintCallable, Category = "Status|Boost")
	bool ConsumeBoost(float DeltaSeconds, float ConsumeRate);

	UPROPERTY(BlueprintAssignable, Category = "Status|Events")
	FStatusValueChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Status|Events")
	FStatusValueChangedSignature OnBoostChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Health", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Boost")
	bool bUsesBoost = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|Boost", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bUsesBoost"))
	float MaxBoost = 100.0f;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Runtime")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status|Runtime")
	float CurrentBoost = 100.0f;
};
