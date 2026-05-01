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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MoveSpeed = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BoostSpeedMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float MovementMass = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Movement", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SecondsToReachMaxSpeed = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Build", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BuildSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRange = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Model|Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackDamage = 5.0f;
};
