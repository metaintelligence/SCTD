#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SCTDHUDWidget.generated.h"

class SProgressBar;
class SBox;
class SBorder;
class SScrollBox;
class STextBlock;
class UStatusComponent;

UCLASS()
class SCTD_API USCTDHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetObservedPawn(APawn* NewObservedPawn);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TWeakObjectPtr<APawn> ObservedPawn;
	TWeakObjectPtr<UStatusComponent> ObservedStatus;

	TSharedPtr<SProgressBar> HealthBar;
	TSharedPtr<SProgressBar> BoostBar;
	TSharedPtr<STextBlock> HealthText;
	TSharedPtr<STextBlock> BoostText;
	TSharedPtr<STextBlock> ModeText;
	TSharedPtr<STextBlock> TargetText;
	TSharedPtr<SBox> BuildListBox;
	TSharedPtr<SBorder> BuildDragCaptureBorder;
	TSharedPtr<SScrollBox> BuildScrollBox;
	TArray<TSharedPtr<SBox>> BuildItemBoxes;

	FProgressBarStyle HealthBarStyle;
	FProgressBarStyle BoostBarStyle;
	bool bIsDraggingBuildList = false;
	float LastBuildDragScreenX = 0.0f;
	float BuildScrollOffset = 0.0f;
	float BuildScrollVelocity = 0.0f;

	void RefreshObservedPawn();
	void RefreshValues();
	void RefreshBuildListLayout();
	TSharedRef<SWidget> BuildTurretList();
	TSharedRef<SWidget> BuildTurretCard(const FString& TurretName, const FString& RoleLabel, const FLinearColor& AccentColor);
	FReply HandleBuildListMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply HandleBuildListMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply HandleBuildListMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void TickBuildListInertia(float DeltaTime);
	FText BuildHealthText() const;
	FText BuildBoostText() const;
	FText BuildModeText() const;
};
