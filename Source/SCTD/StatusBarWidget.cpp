#include "StatusBarWidget.h"

#include "StatusComponent.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"

TSharedRef<SWidget> UStatusBarWidget::RebuildWidget()
{
	const float RowSpacing = bShowBoost ? GaugeInnerPadding : 0.0f;
	const float TotalHeight = bShowBoost ? (GaugeHeight * 2.0f) + RowSpacing : GaugeHeight;

	GaugeBarStyle = FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>(TEXT("ProgressBar"));
	GaugeBarStyle.SetBackgroundImage(FSlateNoResource());
	GaugeBarStyle.SetFillImage(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
	GaugeBarStyle.SetMarqueeImage(FSlateNoResource());

	HealthBar = SNew(SProgressBar)
		.Style(&GaugeBarStyle)
		.FillColorAndOpacity(HealthFillColor)
		.Percent(StatusComponent ? StatusComponent->GetHealthRatio() : 0.0f);

	BoostBar = SNew(SProgressBar)
		.Style(&GaugeBarStyle)
		.FillColorAndOpacity(BoostFillColor)
		.Percent(StatusComponent ? StatusComponent->GetBoostRatio() : 0.0f)
		.Visibility(bShowBoost ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);

	TSharedRef<SWidget> BuiltWidget =
		SAssignNew(RootBox, SBox)
		.WidthOverride(GaugeWidth)
		.HeightOverride(TotalHeight)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(HealthGaugeBox, SBox)
				.HeightOverride(GaugeHeight)
				[
					SAssignNew(HealthGaugeBorder, SBorder)
					.Padding(FMargin(GaugeInnerPadding))
					.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f))
					[
						HealthBar.ToSharedRef()
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, RowSpacing, 0.0f, 0.0f)
			[
				SAssignNew(BoostGaugeBox, SBox)
				.HeightOverride(GaugeHeight)
				.Visibility(bShowBoost ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed)
				[
					SAssignNew(BoostGaugeBorder, SBorder)
					.Padding(FMargin(GaugeInnerPadding))
					.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f))
					[
						BoostBar.ToSharedRef()
					]
				]
			]
		];

	Refresh();
	return BuiltWidget;
}

void UStatusBarWidget::SetStatusComponent(UStatusComponent* NewStatusComponent)
{
	StatusComponent = NewStatusComponent;
	Refresh();
}

void UStatusBarWidget::SetShowBoost(bool bNewShowBoost)
{
	bShowBoost = bNewShowBoost;
	if (BoostGaugeBox)
	{
		BoostGaugeBox->SetVisibility(bShowBoost ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
	SetGaugeLayout(GaugeWidth, GaugeHeight, GaugeInnerPadding);
	Refresh();
}

void UStatusBarWidget::SetBarColors(const FLinearColor& NewHealthFillColor, const FLinearColor& NewBoostFillColor)
{
	HealthFillColor = NewHealthFillColor;
	BoostFillColor = NewBoostFillColor;
	if (HealthBar)
	{
		HealthBar->SetFillColorAndOpacity(HealthFillColor);
	}
	if (BoostBar)
	{
		BoostBar->SetFillColorAndOpacity(BoostFillColor);
	}
}

void UStatusBarWidget::SetGaugeLayout(float NewGaugeWidth, float NewGaugeHeight, float NewGaugeInnerPadding)
{
	GaugeWidth = FMath::Max(1.0f, NewGaugeWidth);
	GaugeHeight = FMath::Max(1.0f, NewGaugeHeight);
	GaugeInnerPadding = FMath::Max(0.0f, NewGaugeInnerPadding);

	if (RootBox)
	{
		const float RowSpacing = bShowBoost ? GaugeInnerPadding : 0.0f;
		const float TotalHeight = bShowBoost ? (GaugeHeight * 2.0f) + RowSpacing : GaugeHeight;
		RootBox->SetWidthOverride(GaugeWidth);
		RootBox->SetHeightOverride(TotalHeight);
	}
	if (HealthGaugeBox)
	{
		HealthGaugeBox->SetHeightOverride(GaugeHeight);
	}
	if (BoostGaugeBox)
	{
		BoostGaugeBox->SetHeightOverride(GaugeHeight);
		BoostGaugeBox->SetVisibility(bShowBoost ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
	if (HealthGaugeBorder)
	{
		HealthGaugeBorder->SetPadding(FMargin(GaugeInnerPadding));
	}
	if (BoostGaugeBorder)
	{
		BoostGaugeBorder->SetPadding(FMargin(GaugeInnerPadding));
	}
}

void UStatusBarWidget::Refresh()
{
	if (HealthBar)
	{
		HealthBar->SetPercent(StatusComponent ? StatusComponent->GetHealthRatio() : 0.0f);
	}

	if (BoostBar)
	{
		BoostBar->SetPercent(StatusComponent ? StatusComponent->GetBoostRatio() : 0.0f);
		BoostBar->SetVisibility(bShowBoost ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
}
