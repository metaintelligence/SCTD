#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusDisplayComponent.generated.h"

class UStatusBarWidget;
class UStatusComponent;

UCLASS(Blueprintable, ClassGroup = (SCTD), meta = (BlueprintSpawnableComponent))
class SCTD_API UStatusDisplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusDisplayComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display")
	FVector RelativeOffset = FVector(0.0f, 0.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float VisibleSecondsAfterChange = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display")
	bool bShowBoost = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display|Gauge", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float GaugeWidth = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display|Gauge", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float GaugeHeight = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display|Gauge", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GaugeOffsetPx = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display|Color")
	FString HealthFillHexColor = TEXT("#FF0000");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Display|Color")
	FString BoostFillHexColor = TEXT("#00FF00");

private:
	UPROPERTY(Transient)
	TObjectPtr<UStatusComponent> StatusComponent;

	UPROPERTY(Transient)
	TObjectPtr<UStatusBarWidget> StatusWidget;

	float HideAtTimeSeconds = 0.0f;

	UFUNCTION()
	void HandleHealthChanged(float CurrentValue, float MaxValue);

	UFUNCTION()
	void HandleBoostChanged(float CurrentValue, float MaxValue);

	void ShowTemporarily();
	void RefreshWidget();
	void UpdateWidgetLocation();
	FVector2D GetWidgetSize() const;
	FLinearColor ParseHexColor(const FString& HexColor, const FLinearColor& FallbackColor) const;
};
