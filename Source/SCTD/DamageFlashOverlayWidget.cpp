#include "DamageFlashOverlayWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"

TSharedRef<SWidget> UDamageFlashOverlayWidget::RebuildWidget()
{
	return SAssignNew(OverlayBorder, SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(OverlayColor.CopyWithNewOpacity(OverlayOpacity))
		.Padding(0.0f)
		.Visibility(EVisibility::HitTestInvisible);
}

void UDamageFlashOverlayWidget::SetOverlayColorAndOpacity(const FLinearColor& NewColor, float NewOpacity)
{
	OverlayColor = NewColor;
	OverlayOpacity = FMath::Clamp(NewOpacity, 0.0f, 1.0f);

	if (OverlayBorder)
	{
		OverlayBorder->SetBorderBackgroundColor(OverlayColor.CopyWithNewOpacity(OverlayOpacity));
		OverlayBorder->SetVisibility(OverlayOpacity > 0.0f ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
}
