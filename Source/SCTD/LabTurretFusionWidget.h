#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Model/Repository/SCTDRepositoryTypes.h"
#include "LabTurretFusionWidget.generated.h"

class SScrollBox;
class SEditableTextBox;
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

private:
	UPROPERTY(Transient)
	TObjectPtr<USCTDUserRepository> UserRepository;

	TSharedPtr<SScrollBox> OwnedTurretScrollBox;
	TSharedPtr<SScrollBox> PartsScrollBox;
	TSharedPtr<SVerticalBox> OwnedTurretPanelBox;
	TSharedPtr<SVerticalBox> PreviewContentBox;
	TSharedPtr<SVerticalBox> StatsContentBox;
	TSharedPtr<SEditableTextBox> TurretNameTextBox;

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
	void RefreshAssemblyPreview();
	void RefreshAssemblyStats();
	void RefreshOwnedTurretList();
	void RefreshPartsList();
	void StartNewAssembly();
	bool IsAssemblyComplete() const;
	bool IsMountTypeMatched() const;
	bool CanAddNewTurret() const;
	FString BuildMountTypeText(ESCTDTurretMountType MountType) const;
	FGuid GetOrCreatePrimaryDeckId();
	FGuid GetSelectedDeckId() const;
	FGuid GetOrCreateDeckIdByIndex(int32 DeckIndex);
	FString GetCurrentTurretName() const;
	void ClearAssembly();
};
