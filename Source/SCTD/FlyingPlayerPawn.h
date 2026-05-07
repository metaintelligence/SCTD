#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlyingPlayerPawn.generated.h"

class AHexGridManager;
class ABaseMonster;
class UPlayerModelComponent;
class UPhysicalMaterial;
class USphereComponent;
class UStatusComponent;
class UStatusDisplayComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EPlayerAircraftState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Flying UMETA(DisplayName = "Flying"),
	Attack UMETA(DisplayName = "Attack"),
	Building UMETA(DisplayName = "Building")
};

UCLASS(Blueprintable)
class SCTD_API AFlyingPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AFlyingPlayerPawn();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Flying Player|AI")
	EPlayerAircraftState GetAircraftState() const { return AircraftState; }

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|AI", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float AttackFacingTimeSeconds = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|Debug")
	bool bLogAircraftStateDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Player|HUD")
	bool bEnablePrototypeHUD = true;

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
	float AttackCooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Flying Player|AI")
	EPlayerAircraftState AircraftState = EPlayerAircraftState::Idle;

private:
	static constexpr float BaseMoveSpeed = 200.0f;
	static constexpr float BoundaryBounceRestitution = 0.9f;
	static constexpr float BoundarySkinDistance = 5.0f;

	void CacheHexGridManager();
	FVector GetCameraRelativeInputDirection() const;
	bool IsMovementInputHeld() const;
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
	void UpdateAircraftState(bool bMovementInputHeld);
	void SetAircraftState(EPlayerAircraftState NewState);
	void TickAttack(float DeltaSeconds);
	void UpdateAttackFacing(float DeltaSeconds);
	void ApplyAngularFacingToward(const FVector& TargetLocation, float DeltaSeconds);
	float GetVehicleYaw() const;
	void SetVehicleYaw(float NewYaw);
	void EnsurePrototypeHUDWidget();
	ABaseMonster* FindClosestAttackTarget() const;
	int32 GetTileDistanceToActor(const AActor* Target) const;
	const TCHAR* GetAircraftStateName(EPlayerAircraftState State) const;
	void LogAircraftDebug(const TCHAR* Format, ...) const;

	TWeakObjectPtr<ABaseMonster> CurrentAttackTarget;
	UPROPERTY(Transient)
	TObjectPtr<class USCTDHUDWidget> PrototypeHUDWidget;

	float VisualYawAngularVelocityDegreesPerSecond = 0.0f;
};
