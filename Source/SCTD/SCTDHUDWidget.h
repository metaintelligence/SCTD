#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Model/Repository/SCTDRepositoryTypes.h"
#include "SCTDHUDWidget.generated.h"

class SProgressBar;
class SBox;
class SBorder;
class SScrollBox;
class STextBlock;
class UStatusComponent;
class USCTDUserRepository;
class UTurretStatsPopupWidget;
class ADefenseManager;

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
	TWeakObjectPtr<ADefenseManager> DefenseManager;

	TSharedPtr<SProgressBar> HealthBar;
	TSharedPtr<SProgressBar> BoostBar;
	TSharedPtr<SProgressBar> ConstructionBar;
	TSharedPtr<SBox> ConstructionGaugeBox;
	TSharedPtr<STextBlock> HealthText;
	TSharedPtr<STextBlock> BoostText;
	TSharedPtr<STextBlock> ModeText;
	TSharedPtr<STextBlock> TargetText;
	TSharedPtr<STextBlock> ScrapText;
	TSharedPtr<STextBlock> DefenseTimeText;
	TSharedPtr<STextBlock> MonsterCountText;
	TSharedPtr<STextBlock> LevelText;
	TSharedPtr<STextBlock> ConstructionText;
	TSharedPtr<SBox> BuildListBox;
	TSharedPtr<SBorder> BuildDragCaptureBorder;
	TSharedPtr<SScrollBox> BuildScrollBox;
	TArray<TSharedPtr<SBox>> BuildItemBoxes;
	TArray<FSCTDPreparedTurretRecord> PreparedTurrets;

	UPROPERTY(Transient)
	TObjectPtr<USCTDUserRepository> UserRepository;

	UPROPERTY(Transient)
	TObjectPtr<UTurretStatsPopupWidget> BuildTurretStatsPopupWidget;

	FProgressBarStyle HealthBarStyle;
	FProgressBarStyle BoostBarStyle;
	bool bIsDraggingBuildList = false;
	bool bBuildListDragExceededClickThreshold = false;
	float LastBuildDragScreenX = 0.0f;
	float BuildDragStartScreenX = 0.0f;
	float BuildScrollOffset = 0.0f;
	float BuildScrollVelocity = 0.0f;
	float BuildCardWidth = 112.0f;
	float BuildCardSpacing = 10.0f;
	int32 HoveredBuildTurretIndex = INDEX_NONE;
	int32 BuildTurretStatsPopupIndex = INDEX_NONE;

	void RefreshObservedPawn();
	void RefreshDefenseManager();
	void RefreshValues();
	void RefreshBuildListLayout();
	void RefreshConstructionGaugeLocation();
	void LoadPreparedTurretsFromSelectedDeck();
	TSharedRef<SWidget> BuildTurretList();
	TSharedRef<SWidget> BuildTurretCard(const FString& TurretName, const FString& RoleLabel, const FLinearColor& AccentColor, int32 TurretIndex);
	FReply HandleBuildListMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply HandleBuildListMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply HandleBuildListMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void TickBuildListInertia(float DeltaTime);
	void RefreshBuildTurretStatsPopupHover();
	int32 GetBuildTurretIndexAtScreenPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPosition) const;
	void ShowBuildTurretStatsPopup(int32 TurretIndex);
	void HideBuildTurretStatsPopup();
	void UpdateBuildTurretStatsPopupPosition();
	TSharedRef<SWidget> BuildLevelUpChoiceOverlay();
	TSharedRef<SWidget> BuildLevelUpCard(int32 CardIndex);
	TSharedRef<SWidget> BuildDefenseResultOverlay();
	TSharedRef<SWidget> BuildDefenseDamageTable();
	TSharedRef<SWidget> BuildDefenseRecordTable();
	FReply HandleLevelUpCardClicked(int32 CardIndex);
	FReply HandleDefenseResultConfirmClicked();
	FText BuildHealthText() const;
	FText BuildBoostText() const;
	FText BuildModeText() const;
	FText BuildScrapText() const;
	FText BuildDefenseTimeText() const;
	FText BuildMonsterCountText() const;
	FText BuildLevelText() const;
	FText BuildLevelUpCardText(int32 CardIndex) const;
	FText BuildDefenseResultTitleText() const;
	FText BuildDefenseResultScrapText() const;
	FText BuildConstructionText() const;
};
