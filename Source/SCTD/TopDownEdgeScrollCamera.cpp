#include "TopDownEdgeScrollCamera.h"

#include "GameFramework/PlayerController.h"

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
}

void ATopDownEdgeScrollCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEnableEdgeScroll || EdgeScrollSpeed <= 0.0f || EdgeScrollMarginPixels <= 0.0f)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	if (MouseX < 0.0f || MouseY < 0.0f || MouseX > ViewportSizeX || MouseY > ViewportSizeY)
	{
		return;
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

	FVector MoveDirection = TopEdgeWorldDirection.GetSafeNormal() * VerticalInput
		+ RightEdgeWorldDirection.GetSafeNormal() * HorizontalInput;

	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	MoveDirection = MoveDirection.GetClampedToMaxSize(1.0f);

	FVector NewLocation = GetActorLocation() + MoveDirection * EdgeScrollSpeed * DeltaSeconds;
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
