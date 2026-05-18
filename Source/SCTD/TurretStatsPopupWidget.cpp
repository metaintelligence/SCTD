#include "TurretStatsPopupWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "SCTDMarqueeText.h"

void UTurretStatsPopupWidget::SetStats(const FSCTDTurretPopupStats& NewStats)
{
	Stats = NewStats;
}

TSharedRef<SWidget> UTurretStatsPopupWidget::RebuildWidget()
{
	const FLinearColor AccentColor(0.0f, 0.92f, 0.84f, 0.95f);
	const FLinearColor PanelColor(0.012f, 0.018f, 0.020f, 0.96f);

	return SNew(SBox)
		.WidthOverride(270.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(AccentColor)
			.Padding(2.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(PanelColor)
				.Padding(FMargin(14.0f, 12.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(Stats.DisplayName.IsEmpty() ? TEXT("TURRET") : Stats.DisplayName))
						.ColorAndOpacity(FLinearColor(0.78f, 1.0f, 0.94f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						BuildStatRow(TEXT("BODY"), Stats.BasePartName)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("WEAPON"), Stats.WeaponPartName)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("CONTROL"), Stats.ControlPartName)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						BuildStatRow(TEXT("HP"), FString::Printf(TEXT("%.0f"), Stats.MaxHealth))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("DEF"), FString::Printf(TEXT("%.0f"), Stats.Defense))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("ATK"), FString::Printf(TEXT("%.0f-%.0f"), Stats.MinAttackDamage, Stats.MaxAttackDamage))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("TYPE"), BuildAttackAttributeText())
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("SPD"), FString::Printf(TEXT("%.2f / sec"), Stats.AttackSpeed))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("RANGE"), FString::Printf(TEXT("%.0f tiles"), Stats.AttackRangeTiles))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						BuildStatRow(TEXT("AI"), BuildAIText())
					]
				]
			]
		];
}

TSharedRef<SWidget> UTurretStatsPopupWidget::BuildStatRow(const FString& Label, const FString& Value) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(FLinearColor(0.38f, 0.78f, 0.78f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(Value.IsEmpty() ? TEXT("-") : Value))
			.ColorAndOpacity(FLinearColor(0.78f, 1.0f, 0.94f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		];
}

FString UTurretStatsPopupWidget::BuildAIText() const
{
	if (Stats.AIProfileId == TEXT("MaxHealth"))
	{
		return TEXT("Max health target");
	}
	if (Stats.AIProfileId == TEXT("MinHealth"))
	{
		return TEXT("Min health target");
	}
	return TEXT("Nearest target");
}

FString UTurretStatsPopupWidget::BuildAttackAttributeText() const
{
	switch (Stats.AttackAttribute)
	{
	case ESCTDAttackAttribute::Physical:
		return TEXT("Physical");
	case ESCTDAttackAttribute::Fire:
		return TEXT("Fire");
	case ESCTDAttackAttribute::Lightning:
		return TEXT("Lightning");
	case ESCTDAttackAttribute::Frost:
		return TEXT("Frost");
	default:
		return TEXT("-");
	}
}
