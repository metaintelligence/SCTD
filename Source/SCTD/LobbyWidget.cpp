#include "LobbyWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> ULobbyWidget::RebuildWidget()
{
	const FLinearColor BackgroundColor(0.004f, 0.006f, 0.008f, 1.0f);

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(BackgroundColor)
		]
		+ SOverlay::Slot()
		.Padding(FMargin(36.0f, 32.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("SCTD COMMAND LOBBY")))
				.ColorAndOpacity(FLinearColor(0.86f, 0.92f, 0.90f, 1.0f))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.72f)
			.Padding(0.0f, 28.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					BuildModeCard(
						TEXT("TOWER DEFENSE"),
						TEXT("Start defense scene"),
						FLinearColor(1.0f, 0.05f, 0.06f, 1.0f),
						FOnClicked::CreateUObject(this, &ULobbyWidget::HandleDefenseClicked))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(52.0f, 0.0f)
				[
					BuildModeCard(
						TEXT("STAT"),
						TEXT("Not implemented yet"),
						FLinearColor(0.00f, 0.70f, 0.25f, 1.0f),
						FOnClicked::CreateUObject(this, &ULobbyWidget::HandleStatClicked))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.86f)
				[
					BuildModeCard(
						TEXT("LAB"),
						TEXT("Turret fusion lab"),
						FLinearColor(0.70f, 0.25f, 0.82f, 1.0f),
						FOnClicked::CreateUObject(this, &ULobbyWidget::HandleLabClicked))
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.28f)
			.Padding(0.0f, 32.0f, 0.0f, 0.0f)
			[
				BuildPlayerStatusPanel()
			]
		];
}

TSharedRef<SWidget> ULobbyWidget::BuildModeCard(const FString& Title, const FString& Subtitle, const FLinearColor& AccentColor, const FOnClicked& OnClicked)
{
	return SNew(SButton)
		.ButtonColorAndOpacity(FLinearColor(0.012f, 0.014f, 0.018f, 1.0f))
		.ContentPadding(FMargin(0.0f))
		.OnClicked(OnClicked)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(AccentColor)
			.Padding(3.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.020f, 0.021f, 0.025f, 0.96f))
				.Padding(FMargin(18.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Title))
						.ColorAndOpacity(AccentColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Subtitle))
						.ColorAndOpacity(FLinearColor(0.72f, 0.76f, 0.78f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];
}

TSharedRef<SWidget> ULobbyWidget::BuildPlayerStatusPanel()
{
	const FLinearColor AccentColor(0.18f, 0.22f, 0.86f, 1.0f);

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(AccentColor)
		.Padding(3.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(0.018f, 0.020f, 0.026f, 0.96f))
			.Padding(FMargin(18.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("PLAYER STATUS")))
					.ColorAndOpacity(FLinearColor(0.54f, 0.60f, 1.0f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Player data model is not planned yet.")))
					.ColorAndOpacity(FLinearColor(0.40f, 0.44f, 0.50f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.Justification(ETextJustify::Center)
				]
			]
		];
}

FReply ULobbyWidget::HandleDefenseClicked()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/SCTD_initial_map")));
	return FReply::Handled();
}

FReply ULobbyWidget::HandleStatClicked()
{
	return FReply::Handled();
}

FReply ULobbyWidget::HandleLabClicked()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/LAB")));
	return FReply::Handled();
}
