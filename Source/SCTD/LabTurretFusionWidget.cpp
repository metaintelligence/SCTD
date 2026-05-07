#include "LabTurretFusionWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> ULabTurretFusionWidget::RebuildWidget()
{
	const FLinearColor BackgroundColor(0.003f, 0.006f, 0.008f, 1.0f);

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(BackgroundColor)
		]
		+ SOverlay::Slot()
		.Padding(FMargin(8.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.24f)
			[
				BuildOwnedTurretList()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.54f)
			.Padding(12.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(0.70f)
				[
					BuildPreviewPanel()
				]
				+ SVerticalBox::Slot()
				.FillHeight(0.30f)
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					BuildStatsPanel()
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.20f)
			[
				BuildPartsPanel()
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildOwnedTurretList() const
{
	const FLinearColor AccentColor(1.0f, 0.05f, 0.06f, 1.0f);
	const FLinearColor PanelColor(0.010f, 0.012f, 0.014f, 0.92f);

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(AccentColor)
		.Padding(2.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(PanelColor)
			.Padding(FMargin(14.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("OWNED TURRETS")))
					.ColorAndOpacity(FLinearColor(1.0f, 0.34f, 0.34f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					SNew(SScrollBox)
					.Orientation(Orient_Vertical)
					.ScrollBarVisibility(EVisibility::Collapsed)
					.AllowOverscroll(EAllowOverscroll::Yes)
					+ SScrollBox::Slot()
					[
						BuildPlusTurretItem()
					]
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPreviewPanel() const
{
	return BuildEmptyPanel(
		TEXT("TURRET BUILD PREVIEW"),
		TEXT("Preview viewport reserved. No turret model is selected yet."),
		FLinearColor(0.00f, 0.76f, 0.25f, 1.0f));
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPartsPanel() const
{
	const FLinearColor AccentColor(0.70f, 0.25f, 0.82f, 1.0f);
	const FLinearColor PanelColor(0.010f, 0.010f, 0.014f, 0.92f);

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(AccentColor)
		.Padding(2.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(PanelColor)
			.Padding(FMargin(12.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						BuildPartTab(TEXT("BODY"), true)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(6.0f, 0.0f)
					[
						BuildPartTab(TEXT("WEAPON"), false)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						BuildPartTab(TEXT("CONTROL"), false)
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					SNew(SScrollBox)
					.Orientation(Orient_Vertical)
					.ScrollBarVisibility(EVisibility::Collapsed)
					.AllowOverscroll(EAllowOverscroll::Yes)
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildStatsPanel() const
{
	return BuildEmptyPanel(
		TEXT("TURRET STATS"),
		TEXT("Stats output reserved for backend data: attack, defense, range, rate, utility."),
		FLinearColor(0.00f, 0.58f, 0.82f, 1.0f));
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPlusTurretItem() const
{
	return SNew(SButton)
		.ButtonColorAndOpacity(FLinearColor(0.08f, 0.014f, 0.018f, 1.0f))
		.ContentPadding(FMargin(0.0f))
		[
			SNew(SBox)
			.HeightOverride(132.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(1.0f, 0.05f, 0.06f, 0.32f))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor(FLinearColor(0.018f, 0.018f, 0.020f, 0.96f))
					.Padding(FMargin(12.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("+")))
							.ColorAndOpacity(FLinearColor(1.0f, 0.34f, 0.34f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
							.Justification(ETextJustify::Center)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("CREATE TURRET")))
							.ColorAndOpacity(FLinearColor(0.72f, 0.38f, 0.38f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPartTab(const FString& Label, bool bSelected) const
{
	const FLinearColor SelectedColor(0.70f, 0.25f, 0.82f, 0.92f);
	const FLinearColor IdleColor(0.12f, 0.07f, 0.14f, 0.95f);

	return SNew(SButton)
		.ButtonColorAndOpacity(bSelected ? SelectedColor : IdleColor)
		.ContentPadding(FMargin(8.0f, 10.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(FLinearColor(0.92f, 0.82f, 0.96f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildEmptyPanel(const FString& Label, const FString& Description, const FLinearColor& AccentColor) const
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(AccentColor)
		.Padding(2.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.008f, 0.010f, 0.012f, 0.94f))
			.Padding(FMargin(18.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.ColorAndOpacity(AccentColor)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Description))
					.ColorAndOpacity(FLinearColor(0.42f, 0.48f, 0.52f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.Justification(ETextJustify::Center)
				]
			]
		];
}
