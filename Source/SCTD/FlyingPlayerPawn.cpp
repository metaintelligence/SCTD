#include "FlyingPlayerPawn.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HexGridManager.h"
#include "InputCoreTypes.h"
#include "PlayerModelComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "StatusComponent.h"
#include "StatusDisplayComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCTDPlayer, Log, All);

AFlyingPlayerPawn::AFlyingPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	FlightBody = CreateDefaultSubobject<USphereComponent>(TEXT("FlightBody"));
	SetRootComponent(FlightBody);
	FlightBody->SetMobility(EComponentMobility::Movable);
	FlightBody->InitSphereRadius(CollisionRadius);
	FlightBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FlightBody->SetCollisionProfileName(TEXT("PhysicsActor"));
	FlightBody->SetSimulatePhysics(true);
	FlightBody->SetEnableGravity(false);
	FlightBody->SetLinearDamping(LinearDamping);
	FlightBody->SetAngularDamping(4.0f);
	FlightBody->BodyInstance.bLockZTranslation = true;
	FlightBody->BodyInstance.bLockXRotation = true;
	FlightBody->BodyInstance.bLockYRotation = true;
	FlightBody->BodyInstance.bLockZRotation = true;

	VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
	VehicleMesh->SetupAttachment(FlightBody);
	VehicleMesh->SetMobility(EComponentMobility::Movable);
	VehicleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VehicleMesh->SetSimulatePhysics(false);

	PlayerModel = CreateDefaultSubobject<UPlayerModelComponent>(TEXT("PlayerModel"));

	StatusComponent = CreateDefaultSubobject<UStatusComponent>(TEXT("StatusComponent"));
	StatusComponent->MaxHealth = 100.0f;
	StatusComponent->bUsesBoost = true;
	StatusComponent->MaxBoost = 100.0f;

	StatusDisplayComponent = CreateDefaultSubobject<UStatusDisplayComponent>(TEXT("StatusDisplayComponent"));
	StatusDisplayComponent->RelativeOffset = FVector(0.0f, 0.0f, 40.0f);
	StatusDisplayComponent->bShowBoost = true;
}

void AFlyingPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	CacheHexGridManager();
	ConfigureVisualMeshAttachment();
	MaintainFlightAltitude();

	if (FlightBody)
	{
		FlightBody->SetSphereRadius(CollisionRadius);
		FlightBody->SetMobility(EComponentMobility::Movable);
		FlightBody->SetLinearDamping(LinearDamping);
		FlightBody->SetMassOverrideInKg(NAME_None, GetMovementMass(), true);
		FlightBody->SetSimulatePhysics(true);
		FlightBody->SetEnableGravity(false);
		FlightBody->WakeAllRigidBodies();

		RuntimePhysicsMaterial = NewObject<UPhysicalMaterial>(this, TEXT("FlyingPlayerPhysicsMaterial"));
		if (RuntimePhysicsMaterial)
		{
			RuntimePhysicsMaterial->Restitution = CollisionRestitution;
			RuntimePhysicsMaterial->Friction = 0.1f;
			FlightBody->SetPhysMaterialOverride(RuntimePhysicsMaterial);
		}
	}

	if (CanMoveToLocation(GetActorLocation()))
	{
		LastTraversableLocation = GetActorLocation();
		bHasLastTraversableLocation = true;
	}
}

void AFlyingPlayerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MaintainFlightAltitude();
	ConfigureVisualMeshAttachment();

	const FVector MoveDirection = GetCameraRelativeInputDirection();
	const bool bBoosting = PlayerModel && StatusComponent && WantsBoost() && StatusComponent->GetCurrentBoost() > 0.0f;
	const float MaxMoveSpeed = GetMaxMoveSpeed(bBoosting);

	if (MoveDirection.IsNearlyZero())
	{
		ResolveTraversabilityBoundary(DeltaSeconds, MaxMoveSpeed);
		if (StatusComponent)
		{
			StatusComponent->RecoverBoost(DeltaSeconds, BoostRecoveryRate);
		}
		ClampHorizontalSpeed(MaxMoveSpeed);
		return;
	}

	if (StatusComponent && bBoosting)
	{
		StatusComponent->ConsumeBoost(DeltaSeconds, BoostConsumeRate);
	}

	ApplyMovementForce(MoveDirection, bBoosting);
	ClampHorizontalSpeed(MaxMoveSpeed);
	ResolveTraversabilityBoundary(DeltaSeconds, MaxMoveSpeed);

	if (StatusComponent)
	{
		if (!bBoosting)
		{
			StatusComponent->RecoverBoost(DeltaSeconds, BoostRecoveryRate);
		}
	}

	if (bAlignToMovementDirection)
	{
		const FRotator TargetRotation = MoveDirection.Rotation();
		const FRotator CurrentRotation = VehicleMesh ? VehicleMesh->GetComponentRotation() : GetActorRotation();
		const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, RotationInterpSpeed);

		if (VehicleMesh)
		{
			VehicleMesh->SetWorldRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
		}
		else
		{
			SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
		}
	}

}

float AFlyingPlayerPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (StatusComponent)
	{
		StatusComponent->ApplyDamage(AppliedDamage > 0.0f ? AppliedDamage : DamageAmount);
	}
	UE_LOG(LogSCTDPlayer, Log, TEXT("%s took damage: requested=%.2f applied=%.2f health=%.2f"),
		*GetNameSafe(this),
		DamageAmount,
		AppliedDamage,
		StatusComponent ? StatusComponent->GetCurrentHealth() : -1.0f);
	return AppliedDamage;
}

void AFlyingPlayerPawn::CacheHexGridManager()
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

FVector AFlyingPlayerPawn::GetCameraRelativeInputDirection() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return FVector::ZeroVector;
	}

	float HorizontalInput = 0.0f;
	HorizontalInput += PlayerController->IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f;
	HorizontalInput -= PlayerController->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f;

	float VerticalInput = 0.0f;
	VerticalInput += PlayerController->IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f;
	VerticalInput -= PlayerController->IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f;

	if (FMath::IsNearlyZero(HorizontalInput) && FMath::IsNearlyZero(VerticalInput))
	{
		return FVector::ZeroVector;
	}

	FRotator CameraYawRotation = FRotator::ZeroRotator;
	if (const AActor* ViewTarget = PlayerController->GetViewTarget())
	{
		CameraYawRotation.Yaw = ViewTarget->GetActorRotation().Yaw;
	}
	else if (PlayerController->PlayerCameraManager)
	{
		CameraYawRotation.Yaw = PlayerController->PlayerCameraManager->GetCameraRotation().Yaw;
	}

	const FVector Forward = FRotationMatrix(CameraYawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(CameraYawRotation).GetUnitAxis(EAxis::Y);

	return (Forward * VerticalInput + Right * HorizontalInput).GetSafeNormal();
}

bool AFlyingPlayerPawn::WantsBoost() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return false;
	}

	return PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift);
}

float AFlyingPlayerPawn::GetMaxMoveSpeed(bool bBoosting) const
{
	const float StatMoveSpeed = PlayerModel ? PlayerModel->MoveSpeed : 30.0f;
	const float BoostMultiplier = bBoosting && PlayerModel ? PlayerModel->BoostSpeedMultiplier : 1.0f;
	return BaseMoveSpeed * StatMoveSpeed * BoostMultiplier;
}

float AFlyingPlayerPawn::GetMovementMass() const
{
	return PlayerModel ? FMath::Max(0.1f, PlayerModel->MovementMass) : 100.0f;
}

float AFlyingPlayerPawn::GetAccelerationToReachMaxSpeed(bool bBoosting) const
{
	const float SecondsToReachMaxSpeed = PlayerModel ? PlayerModel->SecondsToReachMaxSpeed : 0.25f;
	return GetMaxMoveSpeed(bBoosting) / FMath::Max(0.01f, SecondsToReachMaxSpeed);
}

void AFlyingPlayerPawn::ApplyMovementForce(const FVector& MoveDirection, bool bBoosting)
{
	if (!FlightBody || MoveDirection.IsNearlyZero())
	{
		return;
	}

	FVector CurrentVelocity = FlightBody->GetPhysicsLinearVelocity();
	CurrentVelocity.Z = 0.0f;

	const float MaxSpeed = GetMaxMoveSpeed(bBoosting);
	const float SpeedAlongInput = FVector::DotProduct(CurrentVelocity, MoveDirection);
	if (SpeedAlongInput >= MaxSpeed)
	{
		return;
	}

	const float ForceMagnitude = GetMovementMass() * GetAccelerationToReachMaxSpeed(bBoosting);
	FlightBody->AddForce(MoveDirection * ForceMagnitude);
}

void AFlyingPlayerPawn::ClampHorizontalSpeed(float MaxSpeed)
{
	if (!FlightBody)
	{
		return;
	}

	FVector Velocity = FlightBody->GetPhysicsLinearVelocity();
	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.0f;
	HorizontalVelocity = HorizontalVelocity.GetClampedToMaxSize(FMath::Max(0.0f, MaxSpeed));

	FlightBody->SetPhysicsLinearVelocity(FVector(HorizontalVelocity.X, HorizontalVelocity.Y, 0.0f));
}

bool AFlyingPlayerPawn::CanMoveToLocation(const FVector& TargetLocation) const
{
	if (!bConstrainToHexGrid)
	{
		return true;
	}

	return HexGridManager && HexGridManager->IsWorldLocationAllyTraversable(TargetLocation);
}

bool AFlyingPlayerPawn::ShouldBlockMovementToward(const FVector& TargetLocation) const
{
	if (!bConstrainToHexGrid)
	{
		return false;
	}

	if (!HexGridManager)
	{
		return false;
	}

	const bool bCurrentLocationIsTraversable = HexGridManager->IsWorldLocationAllyTraversable(GetActorLocation());
	const bool bTargetLocationIsTraversable = HexGridManager->IsWorldLocationAllyTraversable(TargetLocation);
	return bCurrentLocationIsTraversable && !bTargetLocationIsTraversable;
}

void AFlyingPlayerPawn::ResolveTraversabilityBoundary(float DeltaSeconds, float MaxSpeed)
{
	if (!bConstrainToHexGrid || !HexGridManager || !FlightBody)
	{
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	CurrentLocation.Z = FlightAltitude;

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

	FVector Velocity = FlightBody->GetPhysicsLinearVelocity();
	Velocity.Z = 0.0f;
	if (Velocity.IsNearlyZero())
	{
		return;
	}

	FVector PredictedLocation = CurrentLocation + Velocity * FMath::Max(DeltaSeconds, KINDA_SMALL_NUMBER);
	PredictedLocation.Z = FlightAltitude;
	if (HexGridManager->IsWorldLocationAllyTraversable(PredictedLocation))
	{
		return;
	}

	FVector BoundaryNormal = CurrentLocation - PredictedLocation;
	BoundaryNormal.Z = 0.0f;
	BounceFromBoundary(BoundaryNormal.GetSafeNormal(), CurrentLocation, MaxSpeed);
}

void AFlyingPlayerPawn::BounceFromBoundary(const FVector& BoundaryNormal, const FVector& SafeLocation, float MaxSpeed)
{
	if (!FlightBody || BoundaryNormal.IsNearlyZero())
	{
		return;
	}

	FVector Velocity = FlightBody->GetPhysicsLinearVelocity();
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
	CorrectedLocation.Z = FlightAltitude;
	SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::TeleportPhysics);
	FlightBody->SetPhysicsLinearVelocity(FVector(ReflectedVelocity.X, ReflectedVelocity.Y, 0.0f));
}

void AFlyingPlayerPawn::ConfigureVisualMeshAttachment()
{
	if (!VehicleMesh || !FlightBody)
	{
		return;
	}

	VehicleMesh->SetMobility(EComponentMobility::Movable);
	VehicleMesh->SetSimulatePhysics(false);
	VehicleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VehicleMesh->SetUsingAbsoluteLocation(false);
	VehicleMesh->SetUsingAbsoluteRotation(false);
	VehicleMesh->SetUsingAbsoluteScale(false);

	if (VehicleMesh->GetAttachParent() != FlightBody)
	{
		VehicleMesh->AttachToComponent(FlightBody, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void AFlyingPlayerPawn::MaintainFlightAltitude()
{
	FVector Location = GetActorLocation();
	if (FMath::IsNearlyEqual(Location.Z, FlightAltitude))
	{
		return;
	}

	Location.Z = FlightAltitude;
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);

	if (FlightBody)
	{
		FVector Velocity = FlightBody->GetPhysicsLinearVelocity();
		Velocity.Z = 0.0f;
		FlightBody->SetPhysicsLinearVelocity(Velocity);
	}
}
