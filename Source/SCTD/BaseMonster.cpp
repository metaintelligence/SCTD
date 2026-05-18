#include "BaseMonster.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "DefenseManager.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FloatingDamageTextLibrary.h"
#include "FloatingRewardTextWidget.h"
#include "HexGridManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MonsterAnimInstance.h"
#include "MonsterAIBehavior.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "StatusComponent.h"
#include "StatusDisplayComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCTDMonster, Log, All);

namespace
{
constexpr float SideForceHeadOnAngleDegrees = 5.0f;
constexpr float VisualMovementSpeedThresholdRatio = 0.2f;

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
	case EMonsterActionState::Die:
		return TEXT("Die");
	default:
		return TEXT("Unknown");
	}
}

const TCHAR* GetMonsterVisualStateName(EMonsterVisualState VisualState)
{
	switch (VisualState)
	{
	case EMonsterVisualState::Idle:
		return TEXT("Idle");
	case EMonsterVisualState::Moving:
		return TEXT("Moving");
	case EMonsterVisualState::Attacking:
		return TEXT("Attacking");
	case EMonsterVisualState::Die:
		return TEXT("Die");
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

	StatusComponent = CreateDefaultSubobject<UStatusComponent>(TEXT("StatusComponent"));
	StatusComponent->MaxHealth = 40.0f;
	StatusComponent->bUsesBoost = false;

	StatusDisplayComponent = CreateDefaultSubobject<UStatusDisplayComponent>(TEXT("StatusDisplayComponent"));
	StatusDisplayComponent->RelativeOffset = FVector(0.0f, 0.0f, 40.0f);
	StatusDisplayComponent->bShowBoost = false;
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	if (!AIBehavior && AIBehaviorClass)
	{
		AIBehavior = NewObject<UMonsterAIBehavior>(this, AIBehaviorClass);
	}
	if (!MonsterAnimInstanceClass)
	{
		MonsterAnimInstanceClass = UMonsterAnimInstance::StaticClass();
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
	TickDeathState(DeltaSeconds);
	if (ActionState == EMonsterActionState::Die)
	{
		ClampHorizontalSpeed(0.0f);
		return;
	}

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
		RequestVisualState(EMonsterVisualState::Idle);
		ResolveTraversabilityBoundary(DeltaSeconds, MaxSpeed);
		return;
	}

	SetActionState(EMonsterActionState::Moving);
	UpdateSideForceContacts(MoveDirection);
	FVector AccelerationDirection = MoveDirection;
	if (!CachedSideForceDirection.IsNearlyZero() && SideForceAmount > 0.0f)
	{
		AccelerationDirection += CachedSideForceDirection * SideForceAmount;
	}

	if (!AccelerationDirection.IsNearlyZero())
	{
		ApplyMovementForce(AccelerationDirection);
	}

	ClampHorizontalSpeed(MaxSpeed);
	UpdateVisualMovementDecision(MaxSpeed);
	if (bVisualMovementBlocked)
	{
		RequestVisualState(EMonsterVisualState::Idle);
	}
	else
	{
		RequestVisualState(EMonsterVisualState::Moving);
		AlignVisualToDirection(MoveDirection, DeltaSeconds);
	}
	ResolveTraversabilityBoundary(DeltaSeconds, MaxSpeed);
}

void ABaseMonster::ApplyDamageToMonster(float DamageAmount)
{
	if (!StatusComponent || ActionState == EMonsterActionState::Die)
	{
		return;
	}

	StatusComponent->ApplyDamage(DamageAmount);
	if (StatusComponent->GetCurrentHealth() <= 0.0f)
	{
		StartDeath();
	}
}

float ABaseMonster::GetCurrentHealth() const
{
	return StatusComponent ? StatusComponent->GetCurrentHealth() : 0.0f;
}

float ABaseMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const float RequestedDamage = AppliedDamage > 0.0f ? AppliedDamage : DamageAmount;
	const float PreviousHealth = GetCurrentHealth();
	ApplyDamageToMonster(RequestedDamage);
	const float ActualDamage = FMath::Max(0.0f, PreviousHealth - GetCurrentHealth());
	if (ActualDamage > 0.0f)
	{
		SCTDFloatingDamageText::Spawn(this, ActualDamage);

		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ADefenseManager> It(World); It; ++It)
			{
				if (ADefenseManager* DefenseManager = *It)
				{
					DefenseManager->RegisterDamageDealt(DamageCauser, ActualDamage);
					break;
				}
			}
		}
	}
	return ActualDamage;
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
	MonsterMesh->bPauseAnims = false;
	MonsterMesh->bNoSkeletonUpdate = false;
	MonsterMesh->bEnableUpdateRateOptimizations = false;
	MonsterMesh->GlobalAnimRateScale = 1.0f;
	MonsterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	if (MonsterMesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
	{
		MonsterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
	if (MonsterAnimInstanceClass && MonsterMesh->GetAnimClass() != MonsterAnimInstanceClass)
	{
		MonsterMesh->SetAnimInstanceClass(MonsterAnimInstanceClass);
	}
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

	if (!IsAttackMotionState())
	{
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
}

void ABaseMonster::TickDeathState(float DeltaSeconds)
{
	if (ActionState != EMonsterActionState::Die)
	{
		return;
	}

	if (!bDeathFadeStarted)
	{
		DeathElapsedSeconds += DeltaSeconds;
		if (DeathElapsedSeconds >= FMath::Max(0.01f, DeathAnimationDurationSeconds))
		{
			StartDeathFade();
		}
		return;
	}

	DeathFadeElapsedSeconds += DeltaSeconds;
	const float FadeDuration = FMath::Max(0.01f, DeathFadeDurationSeconds);
	const float FadeAlpha = 1.0f - FMath::Clamp(DeathFadeElapsedSeconds / FadeDuration, 0.0f, 1.0f);
	ApplyDeathFadeAlpha(FadeAlpha);
	if (DeathFadeElapsedSeconds >= FadeDuration)
	{
		LogMonsterDebug(TEXT("Death fade finished. Destroying actor."));
		Destroy();
	}
}

void ABaseMonster::StartDeath()
{
	if (ActionState == EMonsterActionState::Die)
	{
		return;
	}

	GrantScrapReward();
	SetActionState(EMonsterActionState::Die);
	PendingAttackTarget.Reset();
	SideForceContacts.Reset();
	CachedSideForceDirection = FVector::ZeroVector;
	bSideForceDecisionPending = false;
	DeathElapsedSeconds = 0.0f;
	DeathFadeElapsedSeconds = 0.0f;
	bDeathFadeStarted = false;

	if (PhysicsBody)
	{
		PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PhysicsBody->SetSimulatePhysics(false);
		PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	}

	ApplyDeathFadeAlpha(1.0f);
	PlayDeathAnimation();
	LogMonsterDebug(TEXT("Death started: animation=%.2fs fade=%.2fs"),
		DeathAnimationDurationSeconds,
		DeathFadeDurationSeconds);
}

void ABaseMonster::GrantScrapReward()
{
	if (bKillRewardsGranted || (ScrapReward <= 0 && ExpReward <= 0))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ADefenseManager> It(World); It; ++It)
	{
		if (ADefenseManager* DefenseManager = *It)
		{
			if (ScrapReward > 0)
			{
				DefenseManager->AddScrap(ScrapReward);
				if (APlayerController* PlayerController = World->GetFirstPlayerController())
				{
					if (UFloatingRewardTextWidget* RewardWidget = CreateWidget<UFloatingRewardTextWidget>(PlayerController, UFloatingRewardTextWidget::StaticClass()))
					{
						RewardWidget->InitializeRewardText(GetActorLocation(), ScrapReward);
						RewardWidget->AddToViewport(130);
					}
				}
			}
			if (ExpReward > 0)
			{
				DefenseManager->AddExperience(ExpReward);
			}
			bKillRewardsGranted = true;
			return;
		}
	}
}

void ABaseMonster::StartDeathFade()
{
	if (bDeathFadeStarted)
	{
		return;
	}

	bDeathFadeStarted = true;
	DeathFadeElapsedSeconds = 0.0f;
	CreateDeathFadeMaterialInstances();
	ApplyDeathFadeAlpha(1.0f);
	LogMonsterDebug(TEXT("Death fade started."));
}

void ABaseMonster::ApplyDeathFadeAlpha(float Alpha)
{
	if (!MonsterMesh)
	{
		return;
	}

	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	if (DeathFadeMaterialInstances.Num() > 0)
	{
		for (UMaterialInstanceDynamic* MaterialInstance : DeathFadeMaterialInstances)
		{
			if (!MaterialInstance)
			{
				continue;
			}

			MaterialInstance->SetScalarParameterValue(TEXT("Opacity"), ClampedAlpha);
			MaterialInstance->SetScalarParameterValue(TEXT("Alpha"), ClampedAlpha);
			MaterialInstance->SetScalarParameterValue(TEXT("FadeAlpha"), ClampedAlpha);
			MaterialInstance->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.45f, 0.45f, 0.45f, ClampedAlpha));
			MaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.45f, 0.45f, 0.45f, ClampedAlpha));
		}
	}
	else
	{
		MonsterMesh->SetScalarParameterValueOnMaterials(TEXT("Opacity"), ClampedAlpha);
		MonsterMesh->SetScalarParameterValueOnMaterials(TEXT("Alpha"), ClampedAlpha);
		MonsterMesh->SetScalarParameterValueOnMaterials(TEXT("FadeAlpha"), ClampedAlpha);
	}
}

void ABaseMonster::CreateDeathFadeMaterialInstances()
{
	DeathFadeMaterialInstances.Reset();
	if (!MonsterMesh || !DeathFadeMaterial)
	{
		return;
	}

	const int32 MaterialCount = MonsterMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(DeathFadeMaterial, this);
		if (!MaterialInstance)
		{
			continue;
		}

		MonsterMesh->SetMaterial(MaterialIndex, MaterialInstance);
		DeathFadeMaterialInstances.Add(MaterialInstance);
	}
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
	const float SafeMaxDamage = FMath::Max(MinAttackDamage, MaxAttackDamage);
	if (!Target || SafeMaxDamage <= 0.0f)
	{
		return;
	}

	FDamageEvent DamageEvent;
	const float SafeMinDamage = FMath::Max(0.0f, FMath::Min(MinAttackDamage, MaxAttackDamage));
	const float RolledDamage = SafeMaxDamage > 0.0f ? FMath::FRandRange(SafeMinDamage, SafeMaxDamage) : 0.0f;
	const float AppliedDamage = Target->TakeDamage(RolledDamage, DamageEvent, GetController(), this);
	LogMonsterDebug(TEXT("Applied attack damage: target=%s requested=%.2f applied=%.2f"),
		*GetNameSafe(Target),
		RolledDamage,
		AppliedDamage);
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

void ABaseMonster::UpdateSideForceContacts(const FVector& MoveDirection)
{
	TArray<TWeakObjectPtr<ABaseMonster>> CurrentContacts;
	UWorld* World = GetWorld();
	if (!World)
	{
		SideForceContacts.Reset();
		CachedSideForceDirection = FVector::ZeroVector;
		bSideForceDecisionPending = false;
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const float OwnRadius = FMath::Max(0.0f, CollisionRadius);

	for (TActorIterator<ABaseMonster> It(World); It; ++It)
	{
		ABaseMonster* OtherMonster = *It;
		if (!OtherMonster || OtherMonster == this || OtherMonster->IsActorBeingDestroyed())
		{
			continue;
		}

		const float CombinedRadius = OwnRadius + FMath::Max(0.0f, OtherMonster->CollisionRadius);
		if (CombinedRadius <= 0.0f)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(CurrentLocation, OtherMonster->GetActorLocation());
		if (DistanceSquared > FMath::Square(CombinedRadius))
		{
			continue;
		}

		CurrentContacts.Add(OtherMonster);
	}

	const bool bContactsChanged = !AreSideForceContactsSame(CurrentContacts);
	if (CurrentContacts.IsEmpty())
	{
		SideForceContacts.Reset();
		CachedSideForceDirection = FVector::ZeroVector;
		bSideForceDecisionPending = false;
		return;
	}

	if (bContactsChanged)
	{
		SideForceContacts = MoveTemp(CurrentContacts);
		bSideForceDecisionPending = true;
	}

	if (!bSideForceDecisionPending)
	{
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	const float DecisionInterval = FMath::Max(0.0f, SideForceDecisionIntervalSeconds);
	if (CurrentTimeSeconds - LastSideForceDecisionTimeSeconds < DecisionInterval)
	{
		return;
	}

	CachedSideForceDirection = CalculateSideForceDirection(MoveDirection);
	LastSideForceDecisionTimeSeconds = CurrentTimeSeconds;
	bSideForceDecisionPending = false;
}

bool ABaseMonster::AreSideForceContactsSame(const TArray<TWeakObjectPtr<ABaseMonster>>& CurrentContacts) const
{
	if (SideForceContacts.Num() != CurrentContacts.Num())
	{
		return false;
	}

	for (const TWeakObjectPtr<ABaseMonster>& CurrentContact : CurrentContacts)
	{
		const ABaseMonster* CurrentMonster = CurrentContact.Get();
		bool bFoundContact = false;
		for (const TWeakObjectPtr<ABaseMonster>& ExistingContact : SideForceContacts)
		{
			if (ExistingContact.Get() == CurrentMonster)
			{
				bFoundContact = true;
				break;
			}
		}

		if (!bFoundContact)
		{
			return false;
		}
	}

	return true;
}

FVector ABaseMonster::CalculateSideForceDirection(const FVector& MoveDirection) const
{
	FVector LocalForwardDirection = FVector::ZeroVector;
	if (bHasVisualFacingYaw)
	{
		LocalForwardDirection = FRotator(0.0f, VisualFacingYaw, 0.0f).Vector();
	}

	if (LocalForwardDirection.IsNearlyZero() && PhysicsBody)
	{
		LocalForwardDirection = PhysicsBody->GetPhysicsLinearVelocity();
		LocalForwardDirection.Z = 0.0f;
	}

	if (LocalForwardDirection.IsNearlyZero())
	{
		LocalForwardDirection = MoveDirection;
		LocalForwardDirection.Z = 0.0f;
	}

	LocalForwardDirection = LocalForwardDirection.GetSafeNormal();
	if (LocalForwardDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector LocalRightDirection = FRotationMatrix(FRotator(0.0f, LocalForwardDirection.Rotation().Yaw, 0.0f)).GetUnitAxis(EAxis::Y);
	FVector SelectedObstacleDirection = FVector::ZeroVector;
	float BestForwardAlignment = -1.0f;

	for (const TWeakObjectPtr<ABaseMonster>& Contact : SideForceContacts)
	{
		const ABaseMonster* OtherMonster = Contact.Get();
		if (!OtherMonster)
		{
			continue;
		}

		FVector ToOther = OtherMonster->GetActorLocation() - CurrentLocation;
		ToOther.Z = 0.0f;
		const float Distance = ToOther.Size();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector ObstacleDirection = ToOther / Distance;
		const float ForwardAlignment = FVector::DotProduct(LocalForwardDirection, ObstacleDirection);
		if (ForwardAlignment <= 0.0f)
		{
			continue;
		}

		if (ForwardAlignment > BestForwardAlignment)
		{
			BestForwardAlignment = ForwardAlignment;
			SelectedObstacleDirection = ObstacleDirection;
		}
	}

	if (SelectedObstacleDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const float ObstacleRightAmount = FVector::DotProduct(SelectedObstacleDirection, LocalRightDirection);
	if (BestForwardAlignment >= FMath::Cos(FMath::DegreesToRadians(SideForceHeadOnAngleDegrees)))
	{
		return LocalRightDirection * (FMath::RandBool() ? 1.0f : -1.0f);
	}

	if (FMath::IsNearlyZero(ObstacleRightAmount))
	{
		return FVector::ZeroVector;
	}

	return LocalRightDirection * -FMath::Sign(ObstacleRightAmount);
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

	const FVector AccelerationDirection = MoveDirection.GetSafeNormal();
	FVector CurrentVelocity = PhysicsBody->GetPhysicsLinearVelocity();
	CurrentVelocity.Z = 0.0f;

	const float MaxSpeed = GetMaxMoveSpeed();
	const float SpeedAlongInput = FVector::DotProduct(CurrentVelocity, AccelerationDirection);
	if (SpeedAlongInput >= MaxSpeed)
	{
		return;
	}

	const float ForceMagnitude = FMath::Max(0.1f, MovementMass) * GetAccelerationToReachMaxSpeed();
	PhysicsBody->AddForce(MoveDirection * ForceMagnitude);
}

void ABaseMonster::UpdateVisualMovementDecision(float MaxSpeed)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	const float DecisionInterval = FMath::Max(0.0f, SideForceDecisionIntervalSeconds);
	if (CurrentTimeSeconds - LastVisualMovementDecisionTimeSeconds < DecisionInterval)
	{
		return;
	}

	FVector HorizontalVelocity = PhysicsBody ? PhysicsBody->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	HorizontalVelocity.Z = 0.0f;
	const float VisualMovementSpeedThreshold = MaxSpeed * VisualMovementSpeedThresholdRatio;
	bVisualMovementBlocked = MaxSpeed <= 0.0f || HorizontalVelocity.Size() <= VisualMovementSpeedThreshold;
	LastVisualMovementDecisionTimeSeconds = CurrentTimeSeconds;
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
	RequestVisualState(EMonsterVisualState::Idle);
}

void ABaseMonster::PlayMovementAnimation()
{
	RequestVisualState(EMonsterVisualState::Moving);
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
	RequestVisualState(EMonsterVisualState::Attacking, SelectedAnimation, PlayRate);
}

void ABaseMonster::PlayDeathAnimation()
{
	if (!MonsterMesh || !DeathAnimation)
	{
		return;
	}

	const float DesiredDuration = FMath::Max(0.01f, DeathAnimationDurationSeconds);
	const float AnimationDuration = FMath::Max(0.01f, DeathAnimation->GetPlayLength());
	const float PlayRate = AnimationDuration / DesiredDuration;
	RequestVisualState(EMonsterVisualState::Die, DeathAnimation, PlayRate);
}

void ABaseMonster::RequestVisualState(EMonsterVisualState NewVisualState, UAnimSequence* OverrideAnimation, float PlayRate)
{
	UAnimSequence* Animation = OverrideAnimation ? OverrideAnimation : GetAnimationForVisualState(NewVisualState);
	if (!Animation)
	{
		return;
	}

	const EMonsterVisualState PreviousVisualState = VisualState;
	VisualState = NewVisualState;
	if (PreviousVisualState != NewVisualState)
	{
		LastVisualStateTransitionTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : LastVisualStateTransitionTimeSeconds;
		LogMonsterDebug(TEXT("Visual state changed: %s -> %s blend=%.2fs"),
			GetMonsterVisualStateName(PreviousVisualState),
			GetMonsterVisualStateName(NewVisualState),
			AnimationTransitionBlendSeconds);
	}

	const bool bLooping = NewVisualState != EMonsterVisualState::Attacking && NewVisualState != EMonsterVisualState::Die;
	PushVisualStateToAnimInstance(Animation, bLooping, PlayRate);
}

UAnimSequence* ABaseMonster::GetAnimationForVisualState(EMonsterVisualState InVisualState) const
{
	switch (InVisualState)
	{
	case EMonsterVisualState::Idle:
		return IdleAnimation;
	case EMonsterVisualState::Moving:
		return WalkingAnimation;
	case EMonsterVisualState::Attacking:
		return nullptr;
	case EMonsterVisualState::Die:
		return DeathAnimation;
	default:
		return nullptr;
	}
}

void ABaseMonster::PushVisualStateToAnimInstance(UAnimSequence* Animation, bool bLooping, float PlayRate)
{
	if (!MonsterMesh || !Animation)
	{
		return;
	}

	if (CurrentAnimation.Get() != Animation)
	{
		if (Animation == WalkingAnimation)
		{
			ConfigureWalkingAnimationRootMotion();
		}
		CurrentAnimation = Animation;
	}

	UAnimInstance* AnimInstance = MonsterMesh->GetAnimInstance();
	UMonsterAnimInstance* MonsterAnimInstance = Cast<UMonsterAnimInstance>(AnimInstance);
	if (!MonsterAnimInstance && MonsterAnimInstanceClass && MonsterMesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
	{
		MonsterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		MonsterMesh->SetAnimInstanceClass(MonsterAnimInstanceClass);
		AnimInstance = MonsterMesh->GetAnimInstance();
		MonsterAnimInstance = Cast<UMonsterAnimInstance>(AnimInstance);
	}

	if (MonsterAnimInstance)
	{
		MonsterAnimInstance->SetMonsterVisualState(VisualState, Animation, bLooping, PlayRate, AnimationTransitionBlendSeconds);
	}

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
