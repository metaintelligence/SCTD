#include "TopDownEdgeScrollCamera.h"

#include "EngineUtils.h"
#include "FlyingPlayerPawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

ATopDownEdgeScrollCamera::ATopDownEdgeScrollCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	SetActorRotation(FRotator(-60.0f, 0.0f, 0.0f));
}

void ATopDownEdgeScrollCamera::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	if (bSetAsPlayerViewTargetOnBeginPlay)
	{
		PlayerController->SetViewTarget(this);
	}

	PlayerController->bShowMouseCursor = bShowMouseCursor;
	PlayerController->bEnableMouseOverEvents = true;
	PlayerController->bEnableClickEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);

	CacheFollowPawn();
	if (bFollowPlayer && bUseInitialOffsetFromPlayer && FollowPawn.IsValid())
	{
		PlayerFollowOffset = GetActorLocation() - FollowPawn->GetActorLocation();
	}
	InitializeFollowZoom();
}

void ATopDownEdgeScrollCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector EdgeScrollMoveDirection = GetEdgeScrollMoveDirection();
	const bool bWantsEdgeScroll = bEnableEdgeScroll && EdgeScrollSpeed > 0.0f && !EdgeScrollMoveDirection.IsNearlyZero();
	if (bFollowPlayer)
	{
		HandleMouseWheelZoom(DeltaSeconds);
		if (!bWantsEdgeScroll)
		{
			ApplyPlayerFollow(DeltaSeconds);
		}
	}

	if (!bWantsEdgeScroll)
	{
		return;
	}

	FVector NewLocation = GetActorLocation() + EdgeScrollMoveDirection * EdgeScrollSpeed * DeltaSeconds;
	if (bClampToBounds)
	{
		NewLocation.X = FMath::Clamp(NewLocation.X, MinXY.X, MaxXY.X);
		NewLocation.Y = FMath::Clamp(NewLocation.Y, MinXY.Y, MaxXY.Y);
	}

	SetActorLocation(NewLocation);
}

void ATopDownEdgeScrollCamera::CacheFollowPawn()
{
	if (FollowPawn.IsValid())
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PlayerController && PlayerController->GetPawn())
	{
		FollowPawn = PlayerController->GetPawn();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AFlyingPlayerPawn> It(World); It; ++It)
	{
		FollowPawn = *It;
		return;
	}
}

FVector ATopDownEdgeScrollCamera::GetFollowTargetLocation() const
{
	return FollowPawn.IsValid() ? FollowPawn->GetActorLocation() + PlayerFollowOffset : GetActorLocation();
}

void ATopDownEdgeScrollCamera::InitializeFollowZoom()
{
	FollowOffsetDirection = PlayerFollowOffset.GetSafeNormal();
	if (FollowOffsetDirection.IsNearlyZero())
	{
		FollowOffsetDirection = FVector(-0.5f, 0.0f, 0.866f).GetSafeNormal();
	}

	const float SafeMinZoomDistance = GetMinZoomDistance();
	const float SafeMaxZoomDistance = GetMaxZoomDistance();
	CurrentZoomDistance = FMath::Lerp(SafeMinZoomDistance, SafeMaxZoomDistance, FMath::Clamp(InitialZoomRatio, 0.0f, 1.0f));
	TargetZoomDistance = CurrentZoomDistance;
	PlayerFollowOffset = FollowOffsetDirection * CurrentZoomDistance;
}

void ATopDownEdgeScrollCamera::HandleMouseWheelZoom(float DeltaSeconds)
{
	if (!bEnableMouseWheelZoom || DeltaSeconds <= 0.0f)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PlayerController)
	{
		if (PlayerController->WasInputKeyJustPressed(EKeys::MouseScrollUp))
		{
			TargetZoomDistance -= GetZoomStepDistance();
		}
		if (PlayerController->WasInputKeyJustPressed(EKeys::MouseScrollDown))
		{
			TargetZoomDistance += GetZoomStepDistance();
		}
	}

	const float SafeMinZoomDistance = GetMinZoomDistance();
	const float SafeMaxZoomDistance = GetMaxZoomDistance();
	TargetZoomDistance = FMath::Clamp(TargetZoomDistance, SafeMinZoomDistance, SafeMaxZoomDistance);

	const float ZoomDisplacement = TargetZoomDistance - CurrentZoomDistance;
	const float ZoomAcceleration = ZoomDisplacement * ZoomSpringStrength - ZoomVelocity * ZoomDamping;
	ZoomVelocity += ZoomAcceleration * DeltaSeconds;
	CurrentZoomDistance = FMath::Clamp(CurrentZoomDistance + ZoomVelocity * DeltaSeconds, SafeMinZoomDistance, SafeMaxZoomDistance);

	if (FMath::IsNearlyEqual(CurrentZoomDistance, SafeMinZoomDistance) || FMath::IsNearlyEqual(CurrentZoomDistance, SafeMaxZoomDistance))
	{
		ZoomVelocity = 0.0f;
	}

	PlayerFollowOffset = FollowOffsetDirection * CurrentZoomDistance;
}

float ATopDownEdgeScrollCamera::GetMinZoomDistance() const
{
	return FMath::Max(1.0f, FMath::Min(MinZoomDistance, MaxZoomDistance));
}

float ATopDownEdgeScrollCamera::GetMaxZoomDistance() const
{
	return FMath::Max(GetMinZoomDistance(), FMath::Max(MinZoomDistance, MaxZoomDistance));
}

float ATopDownEdgeScrollCamera::GetZoomStepDistance() const
{
	return (GetMaxZoomDistance() - GetMinZoomDistance()) * 0.05f;
}

FVector ATopDownEdgeScrollCamera::GetEdgeScrollMoveDirection() const
{
	if (!bEnableEdgeScroll || EdgeScrollSpeed <= 0.0f || EdgeScrollMarginPixels <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return FVector::ZeroVector;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return FVector::ZeroVector;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return FVector::ZeroVector;
	}

	if (MouseX < 0.0f || MouseY < 0.0f || MouseX > ViewportSizeX || MouseY > ViewportSizeY)
	{
		return FVector::ZeroVector;
	}

	const float MarginX = FMath::Min(EdgeScrollMarginPixels, ViewportSizeX * 0.5f);
	const float MarginY = FMath::Min(EdgeScrollMarginPixels, ViewportSizeY * 0.5f);

	float HorizontalInput = 0.0f;
	if (MouseX <= MarginX)
	{
		HorizontalInput = -GetEdgeStrength(MouseX, MarginX);
	}
	else if (MouseX >= ViewportSizeX - MarginX)
	{
		HorizontalInput = GetEdgeStrength(ViewportSizeX - MouseX, MarginX);
	}

	float VerticalInput = 0.0f;
	if (MouseY <= MarginY)
	{
		VerticalInput = GetEdgeStrength(MouseY, MarginY);
	}
	else if (MouseY >= ViewportSizeY - MarginY)
	{
		VerticalInput = -GetEdgeStrength(ViewportSizeY - MouseY, MarginY);
	}

	const FVector MoveDirection = TopEdgeWorldDirection.GetSafeNormal() * VerticalInput
		+ RightEdgeWorldDirection.GetSafeNormal() * HorizontalInput;
	return MoveDirection.GetClampedToMaxSize(1.0f);
}

void ATopDownEdgeScrollCamera::ApplyPlayerFollow(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	CacheFollowPawn();
	if (!FollowPawn.IsValid())
	{
		return;
	}

	FVector TargetLocation = GetFollowTargetLocation();
	if (bClampToBounds)
	{
		TargetLocation.X = FMath::Clamp(TargetLocation.X, MinXY.X, MaxXY.X);
		TargetLocation.Y = FMath::Clamp(TargetLocation.Y, MinXY.Y, MaxXY.Y);
	}

	const FVector Displacement = TargetLocation - GetActorLocation();
	const FVector Acceleration = (Displacement * FollowSpringStrength - CameraVelocity * FollowDamping)
		.GetClampedToMaxSize(FMath::Max(0.0f, MaxFollowAcceleration));

	CameraVelocity += Acceleration * DeltaSeconds;
	CameraVelocity = CameraVelocity.GetClampedToMaxSize(FMath::Max(0.0f, MaxFollowSpeed));

	FVector NewLocation = GetActorLocation() + CameraVelocity * DeltaSeconds;
	if (bClampToBounds)
	{
		NewLocation.X = FMath::Clamp(NewLocation.X, MinXY.X, MaxXY.X);
		NewLocation.Y = FMath::Clamp(NewLocation.Y, MinXY.Y, MaxXY.Y);
	}

	SetActorLocation(NewLocation);
}

float ATopDownEdgeScrollCamera::GetEdgeStrength(float DistanceToEdge, float Margin) const
{
	if (!bUseSmoothEdgeStrength)
	{
		return 1.0f;
	}

	return FMath::Clamp(1.0f - DistanceToEdge / Margin, 0.0f, 1.0f);
}
