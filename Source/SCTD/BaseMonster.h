#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BaseMonster.generated.h"

class AHexGridManager;
class UMonsterAIBehavior;
class UAnimSequence;
class UAnimationAsset;
class UPhysicalMaterial;
class USkeletalMeshComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EMonsterActionState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Moving UMETA(DisplayName = "Moving"),
	AttackPreMotion UMETA(DisplayName = "Attack Pre Motion"),
	AttackPostMotion UMETA(DisplayName = "Attack Post Motion")
};

UCLASS(Abstract, Blueprintable)
class SCTD_API ABaseMonster : public APawn
{
	GENERATED_BODY()

public:
	ABaseMonster();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Monster|Vitals")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Monster|Vitals")
	void ApplyDamageToMonster(float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	EMonsterActionState GetActionState() const { return ActionState; }

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	bool IsAttackCooldownReady() const { return AttackCooldownRemaining <= 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Monster|Combat")
	bool IsTargetInAttackRange(const AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Grid")
	AHexGridManager* GetHexGridManager() const { return HexGridManager; }

	UFUNCTION(BlueprintCallable, Category = "Monster|AI")
	void SetTargetMoveTileWorldLocation(const FVector& TargetMoveTileWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Monster|AI")
	void ClearTargetMoveTile();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PhysicsBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MonsterMesh;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Monster|AI")
	TObjectPtr<UMonsterAIBehavior> AIBehavior;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|AI")
	TSubclassOf<UMonsterAIBehavior> AIBehaviorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MoveSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackDamage = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackCooldownSeconds = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0", UIMin = "0"))
	int32 AttackRangeTileRadius = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackPreMotionMilliseconds = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackPostMotionMilliseconds = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Physics", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float MovementMass = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Physics", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SecondsToReachMaxSpeed = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CollisionRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LinearDamping = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CollisionRestitution = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Grid")
	bool bConstrainToHexGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Grid")
	TObjectPtr<AHexGridManager> HexGridManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Visual")
	bool bAlignToMovementDirection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Visual", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RotationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Visual")
	float VisualForwardYawOffsetDegrees = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
	TObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
	TObjectPtr<UAnimSequence> WalkingAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
	TArray<TObjectPtr<UAnimSequence>> AttackAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Debug")
	bool bLogMonsterDebug = true;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Combat")
	EMonsterActionState ActionState = EMonsterActionState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> RuntimePhysicsMaterial;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Vitals")
	float CurrentHealth = 40.0f;

private:
	static constexpr float BaseMoveSpeed = 200.0f;
	static constexpr float BoundaryBounceRestitution = 0.9f;
	static constexpr float BoundarySkinDistance = 5.0f;

	float AttackCooldownRemaining = 0.0f;
	float AttackMotionRemaining = 0.0f;
	TWeakObjectPtr<AActor> PendingAttackTarget;
	TWeakObjectPtr<UAnimationAsset> CurrentAnimation;
	FVector LastTraversableLocation = FVector::ZeroVector;
	FVector MeshRelativeLocation = FVector::ZeroVector;
	FVector MeshRelativeScale = FVector::OneVector;
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;
	FRotator AttackFacingStartRotation = FRotator::ZeroRotator;
	FRotator AttackFacingTargetRotation = FRotator::ZeroRotator;
	float VisualFacingYaw = 0.0f;
	int32 CurrentMovementSourceTileIndex = INDEX_NONE;
	int32 CurrentMovementTargetTileIndex = INDEX_NONE;
	float AttackPreMotionDuration = 0.0f;
	float AttackPreMotionElapsed = 0.0f;
	bool bHasLastTraversableLocation = false;
	bool bLoggedMissingMoveTarget = false;
	bool bHasCachedMeshRelativeTransform = false;
	bool bHasVisualFacingYaw = false;

	void CacheHexGridManager();
	void ConfigurePhysicsBody();
	void ConfigureVisualMeshAttachment();
	void CacheMeshRelativeTransform();
	void RestoreMeshRelativePositionAndScale();
	void ConfigureAnimationRootMotion();
	void ConfigureWalkingAnimationRootMotion();
	void TickAttackState(float DeltaSeconds);
	void TryStartAttack(AActor* Target);
	void FinishAttackPreMotion();
	void ApplyAttackToTarget(AActor* Target);
	void UpdateAttackFacing(float DeltaSeconds);
	FRotator GetFacingRotationToward(const FVector& TargetLocation) const;
	void UpdateMovementTargetFromCurrentTile();
	int32 GetTileDistanceToTarget(const AActor* Target) const;
	void ApplyMovementForce(const FVector& MoveDirection);
	float GetMaxMoveSpeed() const;
	float GetAccelerationToReachMaxSpeed() const;
	void ClampHorizontalSpeed(float MaxSpeed);
	bool CanMoveToLocation(const FVector& TargetLocation) const;
	void ResolveTraversabilityBoundary(float DeltaSeconds, float MaxSpeed);
	void BounceFromBoundary(const FVector& BoundaryNormal, const FVector& SafeLocation, float MaxSpeed);
	void AlignVisualToDirection(const FVector& MoveDirection, float DeltaSeconds);
	void FaceDirectionImmediately(const FVector& Direction);
	void FaceRotationImmediately(const FRotator& TargetRotation);
	void ApplyVisualYaw(float TargetYaw);
	bool IsAttackMotionState() const;
	void PlayIdleAnimation();
	void PlayMovementAnimation();
	void PlayAttackAnimation();
	void PlayAnimation(UAnimationAsset* Animation, bool bLooping, float PlayRate);
	void SetActionState(EMonsterActionState NewActionState);
	void LogMonsterDebug(const TCHAR* Format, ...) const;
};
