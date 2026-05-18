#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Model/Repository/SCTDRepositoryTypes.h"
#include "InventoryWidget.generated.h"

class SBox;
class SOverlay;
class SScrollBox;
class SUniformGridPanel;
class USCTDUserRepository;

UCLASS()
class SCTD_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetUserRepository(USCTDUserRepository* NewUserRepository);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	static constexpr int32 GridColumnCount = 10;
	static constexpr int32 GridRowCount = 30;
	static constexpr int32 MaxInventoryItemCount = GridColumnCount * GridRowCount;

	UPROPERTY(Transient)
	TObjectPtr<USCTDUserRepository> UserRepository;

	ESCTDTurretPartType SelectedPartType = ESCTDTurretPartType::Base;
	FName SelectedItemDefinitionFilterId = NAME_None;
	FName SelectedMountTypeFilterId = NAME_None;
	TArray<FName> SelectedFilterOptionIds;
	TArray<TSharedPtr<FName>> AvailableItemFilterOptions;
	TArray<TSharedPtr<FName>> AvailableMountTypeFilterOptions;
	TArray<TSharedPtr<FName>> AvailableFilterOptions;
	TMap<FGuid, FString> UsedPartLabels;
	TOptional<FSCTDOwnedTurretPartRecord> HoveredPart;

	TSharedPtr<SUniformGridPanel> InventoryGrid;
	TSharedPtr<SScrollBox> FilterBox;
	TSharedPtr<SBox> HoverCardBox;

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildTabButton(const FString& Label, ESCTDTurretPartType PartType);
	TSharedRef<SWidget> BuildFilterArea();
	TSharedRef<SWidget> BuildEquipmentFilterArea();
	TSharedRef<SWidget> BuildGridArea();
	TSharedRef<SWidget> BuildGridCell(const TOptional<FSCTDOwnedTurretPartRecord>& PartRecord);
	TSharedRef<SWidget> BuildItemCard(const FSCTDOwnedTurretPartRecord& PartRecord) const;
	TSharedRef<SWidget> BuildStatLine(const FString& Text, const FLinearColor& Color, int32 FontSize = 11) const;
	TSharedRef<SWidget> BuildBasicStatsWidget(const FSCTDOwnedTurretPartRecord& PartRecord) const;
	TSharedRef<SWidget> BuildBasicStatRow(const FString& Label, const FString& BaseValue, const FString& CalculatedValue, bool bChanged) const;
	TSharedRef<SWidget> BuildItemFilterCombo();
	TSharedRef<SWidget> BuildMountTypeFilterCombo();
	TSharedRef<SWidget> BuildOptionCombo(int32 FilterIndex);

	FReply HandleBackClicked();
	FReply HandleTabClicked(ESCTDTurretPartType PartType);
	FReply HandleAddFilterClicked();
	void HandleItemFilterChanged(TSharedPtr<FName> NewValue, ESelectInfo::Type SelectInfo);
	void HandleMountTypeFilterChanged(TSharedPtr<FName> NewValue, ESelectInfo::Type SelectInfo);
	void HandleFilterChanged(TSharedPtr<FName> NewValue, ESelectInfo::Type SelectInfo, int32 FilterIndex);
	void HandleItemHovered(FSCTDOwnedTurretPartRecord PartRecord);
	void HandleItemUnhovered();

	void RefreshAll();
	void RefreshEquipmentFilterOptions();
	void RefreshAvailableFilterOptions();
	void RefreshFilterArea();
	void RefreshGrid();
	void RefreshUsedPartLabels();
	TArray<FSCTDOwnedTurretPartRecord> GetFilteredParts() const;
	bool DoesPartPassFilters(const FSCTDOwnedTurretPartRecord& PartRecord) const;
	FString GetPartTypeLabel(ESCTDTurretPartType PartType) const;
	FString GetMountTypeLabel(ESCTDTurretMountType MountType) const;
	FString GetAttackAttributeLabel(ESCTDAttackAttribute AttackAttribute) const;
	FName GetMountTypeFilterId(ESCTDTurretMountType MountType) const;
	ESCTDTurretMountType GetMountTypeFromFilterId(FName MountTypeFilterId) const;
	FString GetTargetingAILabel(ESCTDTargetingAI TargetingAI) const;
	FString GetPartDescription(const FSCTDOwnedTurretPartRecord& PartRecord) const;
	FString GetItemFilterLabel(FName DefinitionId) const;
	FString GetOptionLabel(FName OptionId) const;
	FString BuildOptionValueText(const FSCTDTurretPartOption& Option) const;
	FString GetUsedPartLabel(const FSCTDOwnedTurretPartRecord& PartRecord) const;
};
