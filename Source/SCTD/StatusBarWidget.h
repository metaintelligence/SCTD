#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatusBarWidget.generated.h"

class UProgressBar;
class UStatusComponent;
class SBorder;
class SBox;
class SProgressBar;

UCLASS()
class SCTD_API UStatusBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatusComponent(UStatusComponent* NewStatusComponent);
	void SetShowBoost(bool bNewShowBoost);
	void SetBarColors(const FLinearColor& NewHealthFillColor, const FLinearColor& NewBoostFillColor);
	void SetGaugeLayout(float NewGaugeWidth, float NewGaugeHeight, float NewGaugeInnerPadding);
	void Refresh();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStatusComponent> StatusComponent;

	TSharedPtr<SProgressBar> HealthBar;

	TSharedPtr<SProgressBar> BoostBar;

	TSharedPtr<SBox> RootBox;

	TSharedPtr<SBox> HealthGaugeBox;

	TSharedPtr<SBox> BoostGaugeBox;

	TSharedPtr<SBorder> HealthGaugeBorder;

	TSharedPtr<SBorder> BoostGaugeBorder;

	FProgressBarStyle GaugeBarStyle;
	FLinearColor HealthFillColor = FLinearColor::Red;
	FLinearColor BoostFillColor = FLinearColor::Green;
	float GaugeWidth = 120.0f;
	float GaugeHeight = 12.0f;
	float GaugeInnerPadding = 2.0f;
	bool bShowBoost = false;
};
