#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Model/Repository/SCTDRepositoryTypes.h"
#include "LabTurretFusionWidget.generated.h"

class SScrollBox;
class SEditableTextBox;
class SBox;
class SVerticalBox;
class USCTDUserRepository;

UCLASS()
class SCTD_API ULabTurretFusionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetUserRepository(USCTDUserRepository* NewUserRepository);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<USCTDUserRepository> UserRepository;

	TSharedPtr<SScrollBox> OwnedTurretScrollBox;
	TSharedPtr<SScrollBox> PartsScrollBox;
	TSharedPtr<SVerticalBox> OwnedTurretPanelBox;
	TSharedPtr<SVerticalBox> PreviewContentBox;
	TSharedPtr<SVerticalBox> StatsContentBox;
	TSharedPtr<SEditableTextBox> TurretNameTextBox;
	TSharedPtr<SBox> HoverCardBox;

	ESCTDTurretPartType SelectedPartType = ESCTDTurretPartType::Base;
	int32 SelectedDeckIndex = 0;
	bool bHasSelectedBasePart = false;
	bool bHasSelectedWeaponPart = false;
	bool bHasSelectedControlPart = false;
	bool bIsEditingTurret = false;
	bool bIsViewingTurret = false;
	FGuid EditingDeckId;
	FGuid EditingTurretId;
	FSCTDOwnedTurretPartRecord SelectedBasePart;
	FSCTDOwnedTurretPartRecord SelectedWeaponPart;
	FSCTDOwnedTurretPartRecord SelectedControlPart;
	TOptional<FSCTDOwnedTurretPartRecord> HoveredPart;
	FVector2D HoverCardPosition = FVector2D::ZeroVector;

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildOwnedTurretList();
	TSharedRef<SWidget> BuildDeckTabs();
	TSharedRef<SWidget> BuildDeckTab(int32 DeckIndex);
	TSharedRef<SWidget> BuildPreviewPanel();
	TSharedRef<SWidget> BuildPartsPanel();
	TSharedRef<SWidget> BuildStatsPanel();
	TSharedRef<SWidget> BuildPlusTurretItem();
	TSharedRef<SWidget> BuildPreparedTurretItem(const FGuid& DeckId, const FSCTDPreparedTurretRecord& TurretRecord, int32 TurretIndex, int32 TurretCount);
	TSharedRef<SWidget> BuildPartTab(const FString& Label, ESCTDTurretPartType PartType);
	TSharedRef<SWidget> BuildPartItem(const FSCTDOwnedTurretPartRecord& PartRecord);
	TSharedRef<SWidget> BuildEmptyPanel(const FString& Label, const FString& Description, const FLinearColor& AccentColor);
	TSharedRef<SWidget> BuildStatsSectionTitle(const FString& Label) const;
	TSharedRef<SWidget> BuildStatsRow(const FString& Label, const FString& Value, const FLinearColor& ValueColor = FLinearColor(0.82f, 0.86f, 0.96f, 1.0f)) const;
	TSharedRef<SWidget> BuildStatsFormulaRow(const FString& Label, float CurrentValue, float BaseValue, const FString& Suffix = TEXT(""), int32 DecimalPlaces = 0) const;
	TSharedRef<SWidget> BuildAttributeDamageRow(const FString& Label, float MinBaseDamage, float MaxBaseDamage, float Ratio) const;
	TSharedRef<SWidget> BuildItemViewerCard(const FSCTDOwnedTurretPartRecord& PartRecord) const;
	TSharedRef<SWidget> BuildItemViewerLine(const FString& Text, const FLinearColor& Color, int32 FontSize = 10) const;

	FReply HandleCreateTurretClicked();
	FReply HandleDeckTabClicked(int32 DeckIndex);
	FReply HandlePartTabClicked(ESCTDTurretPartType PartType);
	FReply HandlePartItemDoubleClicked(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FSCTDOwnedTurretPartRecord PartRecord);
	FReply HandleViewTurretClicked(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FGuid DeckId, FSCTDPreparedTurretRecord TurretRecord);
	FReply HandleRegisterTurretClicked();
	FReply HandleEditTurretClicked(FGuid DeckId, FSCTDPreparedTurretRecord TurretRecord);
	FReply HandleDeleteTurretClicked(FGuid DeckId, FGuid TurretInstanceId);
	FReply HandleMoveTurretClicked(FGuid DeckId, FGuid TurretInstanceId, int32 Direction);
	FReply HandleLobbyClicked();
	FReply HandlePartMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FSCTDOwnedTurretPartRecord PartRecord);
	void HandlePartHovered(FSCTDOwnedTurretPartRecord PartRecord);
	void HandlePartUnhovered();
	void RefreshAssemblyPreview();
	void RefreshAssemblyStats();
	void RefreshOwnedTurretList();
	void RefreshPartsList();
	void StartNewAssembly();
	bool IsAssemblyComplete() const;
	bool IsMountTypeMatched() const;
	bool IsPartUsedInDeck(const FGuid& DeckId, const FGuid& PartInstanceId, const FGuid& IgnoredTurretInstanceId = FGuid()) const;
	bool CanUseSelectedPartsInDeck(const FGuid& DeckId) const;
	bool CanAddNewTurret() const;
	FString BuildMountTypeText(ESCTDTurretMountType MountType) const;
	FString BuildAttackAttributeText(ESCTDAttackAttribute AttackAttribute) const;
	FString BuildOptionValueText(const FSCTDTurretPartOption& Option) const;
	FString GetOptionLabel(FName OptionId) const;
	FGuid GetOrCreatePrimaryDeckId();
	FGuid GetSelectedDeckId() const;
	FGuid GetOrCreateDeckIdByIndex(int32 DeckIndex);
	FString GetCurrentTurretName() const;
	void ClearAssembly();
};
