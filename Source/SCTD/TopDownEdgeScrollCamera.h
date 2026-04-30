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

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bEnableEdgeScroll = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera", meta = (ClampMin = "1.0"))
	float EdgeScrollMarginPixels = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera", meta = (ClampMin = "0.0"))
	float EdgeScrollSpeed = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bUseSmoothEdgeStrength = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bSetAsPlayerViewTargetOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera")
	bool bShowMouseCursor = true;

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

private:
	float GetEdgeStrength(float DistanceToEdge, float Margin) const;
};
