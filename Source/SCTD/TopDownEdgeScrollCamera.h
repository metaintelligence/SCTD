#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "TopDownEdgeScrollCamera.generated.h"

UCLASS(Blueprintable)
class SCTD_API ATopDownEdgeScrollCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	ATopDownEdgeScrollCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Top Down Camera|Damage Feedback")
	void PlayDamageFeedback();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bEnableEdgeScroll = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera", meta = (ClampMin = "0.001", ClampMax = "0.5", UIMin = "0.001", UIMax = "0.1"))
	float EdgeScrollMarginViewportRatio = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera", meta = (ClampMin = "0.0"))
	float EdgeScrollSpeed = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bUseSmoothEdgeStrength = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bSetAsPlayerViewTargetOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bShowMouseCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Follow")
	bool bFollowPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Follow")
	bool bUseInitialOffsetFromPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Follow", meta = (EditCondition = "!bUseInitialOffsetFromPlayer"))
	FVector PlayerFollowOffset = FVector(-1000.0f, 0.0f, 1732.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Follow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FollowSpringStrength = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Follow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FollowDamping = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Follow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxFollowSpeed = 30000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Follow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxFollowAcceleration = 40000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Zoom")
	bool bEnableMouseWheelZoom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Zoom", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MinZoomDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Zoom", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxZoomDistance = 30000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Zoom", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float InitialZoomRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Zoom", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ZoomSpringStrength = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Zoom", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ZoomDamping = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	FVector TopEdgeWorldDirection = FVector(1.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	FVector RightEdgeWorldDirection = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera Bounds")
	bool bClampToBounds = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera Bounds", meta = (EditCondition = "bClampToBounds"))
	FVector2D MinXY = FVector2D(-5000.0f, -5000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera Bounds", meta = (EditCondition = "bClampToBounds"))
	FVector2D MaxXY = FVector2D(5000.0f, 5000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Damage Feedback", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageShakeDurationSeconds = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Damage Feedback", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageShakeAmplitude = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Damage Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float LowHealthOverlayStartRatio = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Damage Feedback")
	FString DamageOverlayColorHex = TEXT("#FF0000");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Damage Feedback", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageOverlayFlashDurationSeconds = 0.2f;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UDamageFlashOverlayWidget> DamageFlashOverlayWidget;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> FollowPawn;

	FVector CameraVelocity = FVector::ZeroVector;
	FVector FollowOffsetDirection = FVector(-0.5f, 0.0f, 0.866f);
	float CurrentZoomDistance = 2000.0f;
	float TargetZoomDistance = 2000.0f;
	float ZoomVelocity = 0.0f;
	float DamageShakeRemainingSeconds = 0.0f;
	float DamageOverlayFlashRemainingSeconds = 0.0f;
	float DamageOverlayPeakOpacity = 0.0f;
	bool bWasEdgeScrollingLastFrame = false;
	FVector LastDamageShakeOffset = FVector::ZeroVector;

	void CacheFollowPawn();
	FVector GetFollowTargetLocation() const;
	void ApplyPlayerFollow(float DeltaSeconds);
	void InitializeFollowZoom();
	void HandleMouseWheelZoom(float DeltaSeconds);
	float GetMinZoomDistance() const;
	float GetMaxZoomDistance() const;
	float GetZoomStepDistance() const;
	FVector GetEdgeScrollMoveDirection() const;
	float GetEdgeStrength(float DistanceToEdge, float Margin) const;
	void ClearPreviousDamageShakeOffset();
	void ApplyDamageShake(float DeltaSeconds);
	void EnsureDamageFlashOverlayWidget();
	void UpdateDamageOverlayFlash(float DeltaSeconds);
	float CalculateLowHealthOverlayOpacity();
	FLinearColor ParseHexColor(const FString& HexColor, const FLinearColor& FallbackColor) const;
};
