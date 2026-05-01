#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlyingPlayerPawn.generated.h"

class AHexGridManager;
class UPlayerModelComponent;
class UPhysicalMaterial;
class USphereComponent;
class UStatusComponent;
class UStatusDisplayComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class SCTD_API AFlyingPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AFlyingPlayerPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> FlightBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VehicleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerModelComponent> PlayerModel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatusComponent> StatusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatusDisplayComponent> StatusDisplayComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|Boost", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BoostRecoveryRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|Boost", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BoostConsumeRate = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player", meta = (ClampMin = "0.0"))
	float FlightAltitude = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player")
	bool bAlignToMovementDirection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player")
	bool bConstrainToHexGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player")
	TObjectPtr<AHexGridManager> HexGridManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|Physics", meta = (ClampMin = "0.0"))
	float CollisionRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|Physics", meta = (ClampMin = "0.0"))
	float LinearDamping = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CollisionRestitution = 0.85f;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> RuntimePhysicsMaterial;

	FVector LastTraversableLocation = FVector::ZeroVector;
	bool bHasLastTraversableLocation = false;

private:
	static constexpr float BaseMoveSpeed = 200.0f;
	static constexpr float BoundaryBounceRestitution = 0.9f;
	static constexpr float BoundarySkinDistance = 5.0f;

	void CacheHexGridManager();
	FVector GetCameraRelativeInputDirection() const;
	bool WantsBoost() const;
	float GetMaxMoveSpeed(bool bBoosting) const;
	float GetMovementMass() const;
	float GetAccelerationToReachMaxSpeed(bool bBoosting) const;
	void ApplyMovementForce(const FVector& MoveDirection, bool bBoosting);
	void ClampHorizontalSpeed(float MaxSpeed);
	bool CanMoveToLocation(const FVector& TargetLocation) const;
	bool ShouldBlockMovementToward(const FVector& TargetLocation) const;
	void ResolveTraversabilityBoundary(float DeltaSeconds, float MaxSpeed);
	void BounceFromBoundary(const FVector& BoundaryNormal, const FVector& SafeLocation, float MaxSpeed);
	void ConfigureVisualMeshAttachment();
	void MaintainFlightAltitude();
};
