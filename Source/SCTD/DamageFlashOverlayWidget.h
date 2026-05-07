#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageFlashOverlayWidget.generated.h"

class SBorder;

UCLASS()
class SCTD_API UDamageFlashOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOverlayColorAndOpacity(const FLinearColor& NewColor, float NewOpacity);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SBorder> OverlayBorder;
	FLinearColor OverlayColor = FLinearColor::Red;
	float OverlayOpacity = 0.0f;
};
