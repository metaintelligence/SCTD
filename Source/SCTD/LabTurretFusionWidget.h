#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LabTurretFusionWidget.generated.h"

UCLASS()
class SCTD_API ULabTurretFusionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedRef<SWidget> BuildOwnedTurretList() const;
	TSharedRef<SWidget> BuildPreviewPanel() const;
	TSharedRef<SWidget> BuildPartsPanel() const;
	TSharedRef<SWidget> BuildStatsPanel() const;
	TSharedRef<SWidget> BuildPlusTurretItem() const;
	TSharedRef<SWidget> BuildPartTab(const FString& Label, bool bSelected) const;
	TSharedRef<SWidget> BuildEmptyPanel(const FString& Label, const FString& Description, const FLinearColor& AccentColor) const;
};
