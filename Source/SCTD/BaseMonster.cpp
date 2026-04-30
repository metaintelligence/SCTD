#include "BaseMonster.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HexGridManager.h"
#include "MonsterAIBehavior.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCTDMonster, Log, All);

namespace
{
const TCHAR* GetMonsterActionStateName(EMonsterActionState ActionState)
{
	switch (ActionState)
	{
	case EMonsterActionState::Idle:
		return TEXT("Idle");
	case EMonsterActionState::Moving:
		return TEXT("Moving");
	case EMonsterActionState::AttackPreMotion:
		return TEXT("AttackPreMotion");
	case EMonsterActionState::AttackPostMotion:
		return TEXT("AttackPostMotion");
	default:
		return TEXT("Unknown");
	}
}
}

ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	AIBehaviorClass = UBasicMonsterAIBehavior::StaticClass();

	PhysicsBody = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsBody"));
	SetRootComponent(PhysicsBody);
	PhysicsBody->SetMobility(EComponentMobility::Movable);
	PhysicsBody->InitSphereRadius(CollisionRadius);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBody->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsBody->SetSimulatePhysics(true);
	PhysicsBody->SetEnableGravity(false);
	PhysicsBody->SetLinearDamping(LinearDamping);
	PhysicsBody->SetAngularDamping(4.0f);
	PhysicsBody->BodyInstance.bLockZTranslation = true;
	PhysicsBody->BodyInstance.bLockXRotation = true;
	PhysicsBody->BodyInstance.bLockYRotation = true;
	PhysicsBody->BodyInstance.bLockZRotation = true;

	MonsterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MonsterMesh"));
	MonsterMesh->SetupAttachment(PhysicsBody);
	MonsterMesh->SetMobility(EComponentMobility::Movable);
	MonsterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MonsterMesh->SetSimulatePhysics(false);
	MonsterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	if (!AIBehavior && AIBehaviorClass)
	{
		AIBehavior = NewObject<UMonsterAIBehavior>(this, AIBehaviorClass);
	}

	CacheHexGridManager();
	ConfigureVisualMeshAttachment();
	CacheMeshRelativeTransform();
	ConfigureAnimationRootMotion();
	ConfigurePhysicsBody();
	LogMonsterDebug(TEXT("BeginPlay: MoveSpeed=%.2f MaxSpeed=%.2f Mass=%.2f CollisionRadius=%.2f"),
		MoveSpeed,
		GetMaxMoveSpeed(),
		MovementMass,
		CollisionRadius);
	UpdateMovementTargetFromCurrentTile();
	PlayIdleAnimation();

	if (CanMoveToLocation(GetActorLocation()))
	{
		LastTraversableLocation = GetActorLocation();
		bHasLastTraversableLocation = true;
	}
}

void ABaseMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ConfigureVisualMeshAttachment();
	RestoreMeshRelativePositionAndScale();
	ConfigureAnimationRootMotion();
	TickAttackState(DeltaSeconds);
	UpdateMovementTargetFromCurrentTile();

	const float MaxSpeed = GetMaxMoveSpeed();
	if (IsAttackMotionState())
	{
		ClampHorizontalSpeed(0.0f);
		ResolveTraversabilityBoundary(DeltaSeconds, MaxSpeed);
		return;
	}

	FMonsterAIAction Action;
	if (AIBehavior)
	{
		Action = AIBehavior->DecideAction(this, DeltaSeconds);
	}

	if (Action.MoveDirection.IsNearlyZero() && !Action.AttackTarget && CurrentMovementTargetTileIndex == INDEX_NONE && !bLoggedMissingMoveTarget)
	{
		LogMonsterDebug(TEXT("No movement target: current tile=%d has no valid next movement target."), CurrentMovementSourceTileIndex);
		bLoggedMissingMoveTarget = true;
	}

	if (AttackCooldownRemaining <= 0.0f && IsTargetInAttackRange(Action.AttackTarget))
	{
		TryStartAttack(Action.AttackTarget);
		ClampHorizontalSpeed(0.0f);
		return;
	}

	FVector MoveDirection = Action.MoveDirection;
	MoveDirection.Z = 0.0f;
	MoveDirection = MoveDirection.GetSafeNormal();
	if (MoveDirection.IsNearlyZero())
	{
		if (CurrentMovementTargetTileIndex == INDEX_NONE)
		{
			SetActionState(EMonsterActionState::Idle);
		}
		ClampHorizontalSpeed(0.0f);
		ResolveTraversabilityBoundary(DeltaSeconds, MaxSpeed);
		return;
	}

	SetActionState(EMonsterActionState::Moving);
	if (!MoveDirection.IsNearlyZero())
	{
		ApplyMovementForce(MoveDirection);
	}

	ClampHorizontalSpeed(MaxSpeed);
	AlignVisualToDirection(MoveDirection, DeltaSeconds);
	ResolveTraversabilityBoundary(DeltaSeconds, MaxSpeed);
}

void ABaseMonster::ApplyDamageToMonster(float DamageAmount)
{
	if (DamageAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
}

void ABaseMonster::SetTargetMoveTileWorldLocation(const FVector& TargetMoveTileWorldLocation)
{
	UBasicMonsterAIBehavior* BasicAIBehavior = Cast<UBasicMonsterAIBehavior>(AIBehavior);
	if (!BasicAIBehavior && AIBehaviorClass)
	{
		AIBehavior = NewObject<UMonsterAIBehavior>(this, AIBehaviorClass);
		BasicAIBehavior = Cast<UBasicMonsterAIBehavior>(AIBehavior);
	}

	if (BasicAIBehavior)
	{
		BasicAIBehavior->SetTargetMoveTileWorldLocation(TargetMoveTileWorldLocation);
	}
}

void ABaseMonster::ClearTargetMoveTile()
{
	if (UBasicMonsterAIBehavior* BasicAIBehavior = Cast<UBasicMonsterAIBehavior>(AIBehavior))
	{
		BasicAIBehavior->ClearTargetMoveTile();
	}
}

void ABaseMonster::CacheHexGridManager()
{
	if (HexGridManager)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AHexGridManager> It(World); It; ++It)
	{
		HexGridManager = *It;
		return;
	}
}

void ABaseMonster::ConfigurePhysicsBody()
{
	if (!PhysicsBody)
	{
		return;
	}

	PhysicsBody->SetSphereRadius(CollisionRadius);
	PhysicsBody->SetMobility(EComponentMobility::Movable);
	PhysicsBody->SetLinearDamping(LinearDamping);
	PhysicsBody->SetMassOverrideInKg(NAME_None, FMath::Max(0.1f, MovementMass), true);
	PhysicsBody->SetSimulatePhysics(true);
	PhysicsBody->SetEnableGravity(false);
	PhysicsBody->WakeAllRigidBodies();

	RuntimePhysicsMaterial = NewObject<UPhysicalMaterial>(this, TEXT("MonsterPhysicsMaterial"));
	if (RuntimePhysicsMaterial)
	{
		RuntimePhysicsMaterial->Restitution = CollisionRestitution;
		RuntimePhysicsMaterial->Friction = 0.1f;
		PhysicsBody->SetPhysMaterialOverride(RuntimePhysicsMaterial);
	}
}

void ABaseMonster::ConfigureVisualMeshAttachment()
{
	if (!MonsterMesh || !PhysicsBody)
	{
		return;
	}

	MonsterMesh->SetMobility(EComponentMobility::Movable);
	MonsterMesh->SetSimulatePhysics(false);
	MonsterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MonsterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MonsterMesh->SetUsingAbsoluteLocation(false);
	MonsterMesh->SetUsingAbsoluteRotation(false);
	MonsterMesh->SetUsingAbsoluteScale(false);

	if (MonsterMesh->GetAttachParent() != PhysicsBody)
	{
		MonsterMesh->AttachToComponent(PhysicsBody, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void ABaseMonster::CacheMeshRelativeTransform()
{
	if (!MonsterMesh || bHasCachedMeshRelativeTransform)
	{
		return;
	}

	MeshRelativeLocation = MonsterMesh->GetRelativeLocation();
	MeshRelativeRotation = MonsterMesh->GetRelativeRotation();
	MeshRelativeScale = MonsterMesh->GetRelativeScale3D();
	VisualFacingYaw = GetActorRotation().Yaw;
	bHasVisualFacingYaw = true;
	bHasCachedMeshRelativeTransform = true;
}

void ABaseMonster::RestoreMeshRelativePositionAndScale()
{
	if (!MonsterMesh || !bHasCachedMeshRelativeTransform)
	{
		return;
	}

	MonsterMesh->SetRelativeLocation(MeshRelativeLocation);
	MonsterMesh->SetRelativeScale3D(MeshRelativeScale);
	if (bHasVisualFacingYaw)
	{
		ApplyVisualYaw(VisualFacingYaw);
	}
}

void ABaseMonster::ConfigureAnimationRootMotion()
{
	if (!MonsterMesh)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = MonsterMesh->GetAnimInstance())
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

void ABaseMonster::ConfigureWalkingAnimationRootMotion()
{
	if (!WalkingAnimation)
	{
		return;
	}

	WalkingAnimation->bEnableRootMotion = true;
	WalkingAnimation->RootMotionRootLock = ERootMotionRootLock::AnimFirstFrame;
	WalkingAnimation->bForceRootLock = false;
}

void ABaseMonster::TickAttackState(float DeltaSeconds)
{
	AttackCooldownRemaining = FMath::Max(0.0f, AttackCooldownRemaining - DeltaSeconds);

	if (ActionState == EMonsterActionState::Idle)
	{
		PlayIdleAnimation();
		return;
	}

	if (ActionState == EMonsterActionState::Moving)
	{
		PlayMovementAnimation();
		return;
	}

	AttackMotionRemaining -= DeltaSeconds;
	if (ActionState == EMonsterActionState::AttackPreMotion)
	{
		UpdateAttackFacing(DeltaSeconds);
	}

	if (AttackMotionRemaining > 0.0f)
	{
		return;
	}

	if (ActionState == EMonsterActionState::AttackPreMotion)
	{
		FinishAttackPreMotion();
		return;
	}

	SetActionState(EMonsterActionState::Moving);
	PendingAttackTarget.Reset();
	PlayMovementAnimation();
}

void ABaseMonster::TryStartAttack(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	SetActionState(EMonsterActionState::AttackPreMotion);
	PendingAttackTarget = Target;
	AttackCooldownRemaining = AttackCooldownSeconds;
	AttackPreMotionDuration = FMath::Max(0.01f, AttackPreMotionMilliseconds * 0.001f);
	AttackPreMotionElapsed = 0.0f;
	AttackMotionRemaining = AttackPreMotionDuration;
	AttackFacingTargetRotation = GetFacingRotationToward(Target->GetActorLocation());
	AttackFacingStartRotation = AttackFacingTargetRotation;
	FaceRotationImmediately(AttackFacingTargetRotation);
	LogMonsterDebug(TEXT("Attack target acquired: target=%s cooldown=%.2fs pre=%.2fs post=%.2fs"),
		*GetNameSafe(Target),
		AttackCooldownSeconds,
		AttackPreMotionMilliseconds * 0.001f,
		AttackPostMotionMilliseconds * 0.001f);
	PlayAttackAnimation();
}

void ABaseMonster::FinishAttackPreMotion()
{
	if (AActor* Target = PendingAttackTarget.Get())
	{
		if (IsTargetInAttackRange(Target))
		{
			ApplyAttackToTarget(Target);
		}
	}

	SetActionState(EMonsterActionState::AttackPostMotion);
	AttackMotionRemaining = AttackPostMotionMilliseconds * 0.001f;
}

void ABaseMonster::ApplyAttackToTarget(AActor* Target)
{
	if (!Target || AttackDamage <= 0.0f)
	{
		return;
	}

	FDamageEvent DamageEvent;
	Target->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
}

void ABaseMonster::UpdateAttackFacing(float DeltaSeconds)
{
	if (!bAlignToMovementDirection)
	{
		return;
	}

	if (AActor* Target = PendingAttackTarget.Get())
	{
		AttackFacingTargetRotation = GetFacingRotationToward(Target->GetActorLocation());
	}

	AttackPreMotionElapsed = FMath::Min(AttackPreMotionDuration, AttackPreMotionElapsed + DeltaSeconds);
	FaceRotationImmediately(AttackFacingTargetRotation);
}

FRotator ABaseMonster::GetFacingRotationToward(const FVector& TargetLocation) const
{
	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return FRotator(0.0f, VisualFacingYaw, 0.0f);
	}

	return Direction.Rotation();
}

void ABaseMonster::UpdateMovementTargetFromCurrentTile()
{
	if (!HexGridManager)
	{
		return;
	}

	FHexTileSlot CurrentSlot;
	if (!HexGridManager->FindTileSlotAtWorldLocation(GetActorLocation(), CurrentSlot))
	{
		return;
	}

	if (CurrentMovementSourceTileIndex == CurrentSlot.SlotIndex)
	{
		return;
	}

	const int32 PreviousSourceTileIndex = CurrentMovementSourceTileIndex;
	CurrentMovementSourceTileIndex = CurrentSlot.SlotIndex;
	LogMonsterDebug(TEXT("Tile changed: %d -> %d (q=%d r=%d next=%d)"),
		PreviousSourceTileIndex,
		CurrentSlot.SlotIndex,
		CurrentSlot.Q,
		CurrentSlot.R,
		CurrentSlot.NextMovementTargetTileIndex);

	FVector NextTargetLocation;
	if (CurrentSlot.NextMovementTargetTileIndex != INDEX_NONE
		&& HexGridManager->GetTileWorldLocationBySlotIndex(CurrentSlot.NextMovementTargetTileIndex, NextTargetLocation))
	{
		CurrentMovementTargetTileIndex = CurrentSlot.NextMovementTargetTileIndex;
		bLoggedMissingMoveTarget = false;
		LogMonsterDebug(TEXT("Movement target tile set: source=%d target=%d location=%s"),
			CurrentSlot.SlotIndex,
			CurrentMovementTargetTileIndex,
			*NextTargetLocation.ToCompactString());
		SetTargetMoveTileWorldLocation(NextTargetLocation);
	}
	else
	{
		LogMonsterDebug(TEXT("Movement target cleared: source=%d next=%d is invalid."),
			CurrentSlot.SlotIndex,
			CurrentSlot.NextMovementTargetTileIndex);
		CurrentMovementTargetTileIndex = INDEX_NONE;
		ClearTargetMoveTile();
	}
}

bool ABaseMonster::IsTargetInAttackRange(const AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	const int32 TileDistance = GetTileDistanceToTarget(Target);
	return TileDistance >= 0 && TileDistance < AttackRangeTileRadius;
}

int32 ABaseMonster::GetTileDistanceToTarget(const AActor* Target) const
{
	if (!Target || !HexGridManager)
	{
		return -1;
	}

	FHexTileSlot MonsterSlot;
	FHexTileSlot TargetSlot;
	if (!HexGridManager->FindTileSlotAtWorldLocation(GetActorLocation(), MonsterSlot)
		|| !HexGridManager->FindTileSlotAtWorldLocation(Target->GetActorLocation(), TargetSlot))
	{
		return -1;
	}

	const int32 DeltaQ = MonsterSlot.Q - TargetSlot.Q;
	const int32 DeltaR = MonsterSlot.R - TargetSlot.R;
	const int32 DeltaS = -DeltaQ - DeltaR;
	return FMath::Max3(FMath::Abs(DeltaQ), FMath::Abs(DeltaR), FMath::Abs(DeltaS));
}

void ABaseMonster::ApplyMovementForce(const FVector& MoveDirection)
{
	if (!PhysicsBody || MoveDirection.IsNearlyZero())
	{
		return;
	}

	FVector CurrentVelocity = PhysicsBody->GetPhysicsLinearVelocity();
	CurrentVelocity.Z = 0.0f;

	const float MaxSpeed = GetMaxMoveSpeed();
	const float SpeedAlongInput = FVector::DotProduct(CurrentVelocity, MoveDirection);
	if (SpeedAlongInput >= MaxSpeed)
	{
		return;
	}

	const float ForceMagnitude = FMath::Max(0.1f, MovementMass) * GetAccelerationToReachMaxSpeed();
	PhysicsBody->AddForce(MoveDirection * ForceMagnitude);
}

float ABaseMonster::GetMaxMoveSpeed() const
{
	return BaseMoveSpeed * MoveSpeed;
}

float ABaseMonster::GetAccelerationToReachMaxSpeed() const
{
	return GetMaxMoveSpeed() / FMath::Max(0.01f, SecondsToReachMaxSpeed);
}

void ABaseMonster::ClampHorizontalSpeed(float MaxSpeed)
{
	if (!PhysicsBody)
	{
		return;
	}

	FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.0f;
	HorizontalVelocity = HorizontalVelocity.GetClampedToMaxSize(FMath::Max(0.0f, MaxSpeed));
	PhysicsBody->SetPhysicsLinearVelocity(FVector(HorizontalVelocity.X, HorizontalVelocity.Y, 0.0f));
}

bool ABaseMonster::CanMoveToLocation(const FVector& TargetLocation) const
{
	if (!bConstrainToHexGrid)
	{
		return true;
	}

	return HexGridManager && HexGridManager->IsWorldLocationAllyTraversable(TargetLocation);
}

void ABaseMonster::ResolveTraversabilityBoundary(float DeltaSeconds, float MaxSpeed)
{
	if (!bConstrainToHexGrid || !HexGridManager || !PhysicsBody)
	{
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	const bool bCurrentLocationIsTraversable = HexGridManager->IsWorldLocationAllyTraversable(CurrentLocation);
	if (bCurrentLocationIsTraversable)
	{
		LastTraversableLocation = CurrentLocation;
		bHasLastTraversableLocation = true;
	}
	else if (bHasLastTraversableLocation)
	{
		FVector BoundaryNormal = LastTraversableLocation - CurrentLocation;
		BoundaryNormal.Z = 0.0f;
		BounceFromBoundary(BoundaryNormal.GetSafeNormal(), LastTraversableLocation, MaxSpeed);
		return;
	}

	FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
	Velocity.Z = 0.0f;
	if (Velocity.IsNearlyZero())
	{
		return;
	}

	FVector PredictedLocation = CurrentLocation + Velocity * FMath::Max(DeltaSeconds, KINDA_SMALL_NUMBER);
	if (HexGridManager->IsWorldLocationAllyTraversable(PredictedLocation))
	{
		return;
	}

	FVector BoundaryNormal = CurrentLocation - PredictedLocation;
	BoundaryNormal.Z = 0.0f;
	BounceFromBoundary(BoundaryNormal.GetSafeNormal(), CurrentLocation, MaxSpeed);
}

void ABaseMonster::BounceFromBoundary(const FVector& BoundaryNormal, const FVector& SafeLocation, float MaxSpeed)
{
	if (!PhysicsBody || BoundaryNormal.IsNearlyZero())
	{
		return;
	}

	FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
	Velocity.Z = 0.0f;

	FVector ReflectedVelocity = Velocity - 2.0f * FVector::DotProduct(Velocity, BoundaryNormal) * BoundaryNormal;
	if (ReflectedVelocity.IsNearlyZero())
	{
		ReflectedVelocity = BoundaryNormal * FMath::Max(MaxSpeed * BoundaryBounceRestitution, 0.0f);
	}
	else
	{
		ReflectedVelocity *= BoundaryBounceRestitution;
		ReflectedVelocity = ReflectedVelocity.GetClampedToMaxSize(FMath::Max(MaxSpeed, ReflectedVelocity.Size()));
	}

	FVector CorrectedLocation = SafeLocation + BoundaryNormal * BoundarySkinDistance;
	SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::TeleportPhysics);
	PhysicsBody->SetPhysicsLinearVelocity(FVector(ReflectedVelocity.X, ReflectedVelocity.Y, 0.0f));
}

void ABaseMonster::AlignVisualToDirection(const FVector& MoveDirection, float DeltaSeconds)
{
	if (!bAlignToMovementDirection)
	{
		return;
	}

	FVector FacingDirection = FVector::ZeroVector;
	if (PhysicsBody)
	{
		FacingDirection = PhysicsBody->GetPhysicsLinearVelocity();
		FacingDirection.Z = 0.0f;
	}

	if (FacingDirection.IsNearlyZero())
	{
		FacingDirection = MoveDirection;
		FacingDirection.Z = 0.0f;
	}

	if (FacingDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation = FacingDirection.Rotation();
	const FRotator CurrentRotation(0.0f, VisualFacingYaw, 0.0f);
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, RotationInterpSpeed);
	ApplyVisualYaw(NewRotation.Yaw);
}

void ABaseMonster::FaceDirectionImmediately(const FVector& Direction)
{
	FVector FlatDirection = Direction;
	FlatDirection.Z = 0.0f;
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	FaceRotationImmediately(FlatDirection.Rotation());
}

void ABaseMonster::FaceRotationImmediately(const FRotator& TargetRotation)
{
	if (!bAlignToMovementDirection)
	{
		return;
	}

	ApplyVisualYaw(TargetRotation.Yaw);
}

void ABaseMonster::ApplyVisualYaw(float TargetYaw)
{
	VisualFacingYaw = TargetYaw;
	bHasVisualFacingYaw = true;

	if (MonsterMesh)
	{
		const FQuat DesiredYaw = FRotator(0.0f, VisualFacingYaw + VisualForwardYawOffsetDegrees, 0.0f).Quaternion();
		const FQuat MeshOffset = MeshRelativeRotation.Quaternion();
		MonsterMesh->SetRelativeRotation((DesiredYaw * MeshOffset).Rotator());
	}
}

bool ABaseMonster::IsAttackMotionState() const
{
	return ActionState == EMonsterActionState::AttackPreMotion
		|| ActionState == EMonsterActionState::AttackPostMotion;
}

void ABaseMonster::PlayIdleAnimation()
{
	PlayAnimation(IdleAnimation, true, 1.0f);
}

void ABaseMonster::PlayMovementAnimation()
{
	ConfigureWalkingAnimationRootMotion();
	PlayAnimation(WalkingAnimation, true, 1.0f);
}

void ABaseMonster::PlayAttackAnimation()
{
	if (!MonsterMesh || AttackAnimations.Num() == 0)
	{
		return;
	}

	TArray<UAnimSequence*> ValidAttackAnimations;
	for (UAnimSequence* AttackAnimation : AttackAnimations)
	{
		if (AttackAnimation)
		{
			ValidAttackAnimations.Add(AttackAnimation);
		}
	}

	if (ValidAttackAnimations.Num() == 0)
	{
		return;
	}

	UAnimSequence* SelectedAnimation = ValidAttackAnimations[FMath::RandRange(0, ValidAttackAnimations.Num() - 1)];
	const float DesiredDuration = FMath::Max(0.01f, (AttackPreMotionMilliseconds + AttackPostMotionMilliseconds) * 0.001f);
	const float AnimationDuration = FMath::Max(0.01f, SelectedAnimation->GetPlayLength());
	const float PlayRate = AnimationDuration / DesiredDuration;
	PlayAnimation(SelectedAnimation, false, PlayRate);
}

void ABaseMonster::PlayAnimation(UAnimationAsset* Animation, bool bLooping, float PlayRate)
{
	if (!MonsterMesh || !Animation)
	{
		return;
	}

	if (CurrentAnimation.Get() != Animation)
	{
		MonsterMesh->PlayAnimation(Animation, bLooping);
		CurrentAnimation = Animation;
		ConfigureAnimationRootMotion();
	}

	MonsterMesh->SetPlayRate(FMath::Max(0.01f, PlayRate));
	RestoreMeshRelativePositionAndScale();
}

void ABaseMonster::SetActionState(EMonsterActionState NewActionState)
{
	if (ActionState == NewActionState)
	{
		return;
	}

	const EMonsterActionState PreviousActionState = ActionState;
	ActionState = NewActionState;
	LogMonsterDebug(TEXT("Action state changed: %s -> %s"),
		GetMonsterActionStateName(PreviousActionState),
		GetMonsterActionStateName(NewActionState));
}

void ABaseMonster::LogMonsterDebug(const TCHAR* Format, ...) const
{
	if (!bLogMonsterDebug)
	{
		return;
	}

	TCHAR MessageBuffer[1024];
	va_list Args;
	va_start(Args, Format);
	FCString::GetVarArgs(MessageBuffer, UE_ARRAY_COUNT(MessageBuffer), Format, Args);
	va_end(Args);

	UE_LOG(LogSCTDMonster, Log, TEXT("[%s] %s"), *GetNameSafe(this), MessageBuffer);
}
