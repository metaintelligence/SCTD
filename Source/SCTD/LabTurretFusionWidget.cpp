#include "LabTurretFusionWidget.h"

#include "Model/Repository/SCTDDeckRepository.h"
#include "Model/Repository/SCTDPartsRepository.h"
#include "Model/Repository/SCTDUserRepository.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString PartLabel(const bool bHasPart, const FSCTDOwnedTurretPartRecord& PartRecord, const TCHAR* EmptyLabel)
	{
		return bHasPart ? PartRecord.DisplayName : FString::Printf(TEXT("[%s not selected]"), EmptyLabel);
	}

	FLinearColor PartAccentColor(const ESCTDTurretPartType PartType)
	{
		if (PartType == ESCTDTurretPartType::Base)
		{
			return FLinearColor(0.70f, 0.25f, 0.82f, 1.0f);
		}
		if (PartType == ESCTDTurretPartType::Weapon)
		{
			return FLinearColor(1.0f, 0.50f, 0.18f, 1.0f);
		}
		return FLinearColor(0.20f, 0.72f, 1.0f, 1.0f);
	}
}

void ULabTurretFusionWidget::SetUserRepository(USCTDUserRepository* NewUserRepository)
{
	UserRepository = NewUserRepository;
	RefreshOwnedTurretList();
	RefreshPartsList();
	RefreshAssemblyPreview();
	RefreshAssemblyStats();
}

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
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildHeader()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 10.0f, 0.0f, 0.0f)
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
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildHeader()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(FLinearColor(0.018f, 0.020f, 0.024f, 0.96f))
		.Padding(FMargin(16.0f, 10.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("LAB / TURRET FUSION")))
				.ColorAndOpacity(FLinearColor(0.78f, 0.92f, 0.86f, 1.0f))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ContentPadding(FMargin(18.0f, 7.0f))
				.ButtonColorAndOpacity(FLinearColor(0.05f, 0.16f, 0.13f, 1.0f))
				.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleLobbyClicked))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("TO LOBBY")))
					.ColorAndOpacity(FLinearColor(0.82f, 1.0f, 0.92f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildOwnedTurretList()
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
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					BuildDeckTabs()
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					SAssignNew(OwnedTurretScrollBox, SScrollBox)
					.Orientation(Orient_Vertical)
					.ScrollBarVisibility(EVisibility::Collapsed)
					.AllowOverscroll(EAllowOverscroll::Yes)
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildDeckTabs()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			BuildDeckTab(0)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(6.0f, 0.0f)
		[
			BuildDeckTab(1)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			BuildDeckTab(2)
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildDeckTab(int32 DeckIndex)
{
	return SNew(SButton)
		.ButtonColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda(
			[this, DeckIndex]()
			{
				return FSlateColor(SelectedDeckIndex == DeckIndex
					? FLinearColor(1.0f, 0.12f, 0.14f, 0.92f)
					: FLinearColor(0.14f, 0.035f, 0.040f, 0.95f));
			})))
		.ContentPadding(FMargin(8.0f, 9.0f))
		.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleDeckTabClicked, DeckIndex))
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%d"), DeckIndex + 1)))
			.ColorAndOpacity(FLinearColor(1.0f, 0.80f, 0.80f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPreviewPanel()
{
	const FLinearColor AccentColor(0.00f, 0.76f, 0.25f, 1.0f);

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
				SAssignNew(PreviewContentBox, SVerticalBox)
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPartsPanel()
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
					+ SHorizontalBox::Slot().FillWidth(1.0f)[BuildPartTab(TEXT("BODY"), ESCTDTurretPartType::Base)]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)[BuildPartTab(TEXT("WEAPON"), ESCTDTurretPartType::Weapon)]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[BuildPartTab(TEXT("CONTROL"), ESCTDTurretPartType::Control)]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					SAssignNew(PartsScrollBox, SScrollBox)
					.Orientation(Orient_Vertical)
					.ScrollBarVisibility(EVisibility::Collapsed)
					.AllowOverscroll(EAllowOverscroll::Yes)
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildStatsPanel()
{
	const FLinearColor AccentColor(0.00f, 0.58f, 0.82f, 1.0f);

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
				SAssignNew(StatsContentBox, SVerticalBox)
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPlusTurretItem()
{
	const bool bCanAdd = CanAddNewTurret();
	return SNew(SButton)
		.ButtonColorAndOpacity(bCanAdd ? FLinearColor(0.08f, 0.014f, 0.018f, 1.0f) : FLinearColor(0.035f, 0.035f, 0.038f, 1.0f))
		.ContentPadding(FMargin(0.0f))
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleCreateTurretClicked))
		[
			SNew(SBox)
			.HeightOverride(132.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(1.0f, 0.05f, 0.06f, bCanAdd ? 0.32f : 0.14f))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor(FLinearColor(0.018f, 0.018f, 0.020f, 0.96f))
					.Padding(FMargin(12.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("+")))
							.ColorAndOpacity(bCanAdd ? FLinearColor(1.0f, 0.34f, 0.34f, 1.0f) : FLinearColor(0.34f, 0.34f, 0.36f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
							.Justification(ETextJustify::Center)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(bCanAdd ? TEXT("NEW TURRET") : TEXT("DECK FULL")))
							.ColorAndOpacity(bCanAdd ? FLinearColor(0.72f, 0.38f, 0.38f, 1.0f) : FLinearColor(0.38f, 0.38f, 0.40f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPreparedTurretItem(const FGuid& DeckId, const FSCTDPreparedTurretRecord& TurretRecord, int32 TurretIndex, int32 TurretCount)
{
	return SNew(SBox)
		.HeightOverride(118.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor(1.0f, 0.05f, 0.06f, 0.34f))
			.Padding(1.0f)
			.OnMouseButtonUp(FPointerEventHandler::CreateUObject(this, &ULabTurretFusionWidget::HandleViewTurretClicked, DeckId, TurretRecord))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.018f, 0.018f, 0.020f, 0.96f))
				.Padding(FMargin(12.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%d"), TurretIndex + 1)))
						.ColorAndOpacity(FLinearColor(1.0f, 0.38f, 0.38f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TurretRecord.DisplayName))
							.ColorAndOpacity(FLinearColor(1.0f, 0.62f, 0.62f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("REGISTERED BUILD")))
							.ColorAndOpacity(FLinearColor(0.72f, 0.38f, 0.38f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(82.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(5.0f, 3.0f))
									.IsEnabled(TurretIndex > 0)
									.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleMoveTurretClicked, DeckId, TurretRecord.InstanceId, -1))
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("UP"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									]
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(5.0f, 3.0f))
									.IsEnabled(TurretIndex < TurretCount - 1)
									.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleMoveTurretClicked, DeckId, TurretRecord.InstanceId, 1))
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("DN"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(5.0f, 3.0f))
									.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleEditTurretClicked, DeckId, TurretRecord))
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("EDIT"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									]
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
									.ContentPadding(FMargin(5.0f, 3.0f))
									.ButtonColorAndOpacity(FLinearColor(0.55f, 0.06f, 0.06f, 1.0f))
									.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleDeleteTurretClicked, DeckId, TurretRecord.InstanceId))
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("DEL"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									]
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPartTab(const FString& Label, ESCTDTurretPartType PartType)
{
	const bool bSelected = SelectedPartType == PartType;
	const FLinearColor SelectedColor(0.70f, 0.25f, 0.82f, 0.92f);
	const FLinearColor IdleColor(0.12f, 0.07f, 0.14f, 0.95f);

	return SNew(SButton)
		.ButtonColorAndOpacity(bSelected ? SelectedColor : IdleColor)
		.ContentPadding(FMargin(8.0f, 10.0f))
		.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandlePartTabClicked, PartType))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(FLinearColor(0.92f, 0.82f, 0.96f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPartItem(const FSCTDOwnedTurretPartRecord& PartRecord)
{
	const FLinearColor AccentColor = PartAccentColor(PartRecord.PartType);
	FString StatLine;
	if (PartRecord.PartType == ESCTDTurretPartType::Base)
	{
		StatLine = FString::Printf(TEXT("HP %.0f / DEF %.0f"), PartRecord.BaseHealth, PartRecord.Defense);
	}
	else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
	{
		StatLine = FString::Printf(TEXT("ATK %.0f / SPD %.2f"), PartRecord.AttackDamage, PartRecord.AttackSpeed);
	}
	else
	{
		StatLine = FString::Printf(TEXT("AI %s"), *PartRecord.AIProfileId.ToString());
	}

	return SNew(SBox)
		.HeightOverride(88.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.50f))
			.Padding(1.0f)
			.OnMouseDoubleClick(FPointerEventHandler::CreateUObject(this, &ULabTurretFusionWidget::HandlePartItemDoubleClicked, PartRecord))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.018f, 0.018f, 0.022f, 0.96f))
				.Padding(FMargin(10.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(PartRecord.DisplayName))
						.ColorAndOpacity(FLinearColor(0.92f, 0.82f, 0.96f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(StatLine))
						.ColorAndOpacity(AccentColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("COST %d / TIME %.1fs"), PartRecord.BuildCost, PartRecord.BuildTimeSeconds)))
						.ColorAndOpacity(FLinearColor(0.58f, 0.52f, 0.62f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildEmptyPanel(const FString& Label, const FString& Description, const FLinearColor& AccentColor)
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
				SNew(STextBlock)
				.Text(FText::FromString(Description))
				.ColorAndOpacity(AccentColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
			]
		];
}

FReply ULabTurretFusionWidget::HandleCreateTurretClicked()
{
	if (!CanAddNewTurret())
	{
		return FReply::Handled();
	}

	StartNewAssembly();
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandleDeckTabClicked(int32 DeckIndex)
{
	SelectedDeckIndex = FMath::Clamp(DeckIndex, 0, USCTDDeckRepository::MaxDeckCount - 1);
	GetOrCreateDeckIdByIndex(SelectedDeckIndex);
	if (UserRepository)
	{
		UserRepository->Save();
	}

	ClearAssembly();
	RefreshOwnedTurretList();
	RefreshAssemblyPreview();
	RefreshAssemblyStats();
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandlePartTabClicked(ESCTDTurretPartType PartType)
{
	SelectedPartType = PartType;
	RefreshPartsList();
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandlePartItemDoubleClicked(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FSCTDOwnedTurretPartRecord PartRecord)
{
	if (bIsViewingTurret && !bIsEditingTurret)
	{
		return FReply::Handled();
	}

	if (PartRecord.PartType == ESCTDTurretPartType::Base)
	{
		SelectedBasePart = PartRecord;
		bHasSelectedBasePart = true;
	}
	else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
	{
		SelectedWeaponPart = PartRecord;
		bHasSelectedWeaponPart = true;
	}
	else
	{
		SelectedControlPart = PartRecord;
		bHasSelectedControlPart = true;
	}

	RefreshAssemblyPreview();
	RefreshAssemblyStats();
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandleViewTurretClicked(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FGuid DeckId, FSCTDPreparedTurretRecord TurretRecord)
{
	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!PartsRepository)
	{
		return FReply::Handled();
	}

	bHasSelectedBasePart = PartsRepository->FindPart(TurretRecord.BasePartInstanceId, SelectedBasePart);
	bHasSelectedWeaponPart = PartsRepository->FindPart(TurretRecord.WeaponPartInstanceId, SelectedWeaponPart);
	bHasSelectedControlPart = PartsRepository->FindPart(TurretRecord.ControlPartInstanceId, SelectedControlPart);
	bIsEditingTurret = false;
	bIsViewingTurret = true;
	EditingDeckId = DeckId;
	EditingTurretId = TurretRecord.InstanceId;

	RefreshAssemblyPreview();
	RefreshAssemblyStats();
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandleRegisterTurretClicked()
{
	if (!IsAssemblyComplete() || !UserRepository)
	{
		return FReply::Handled();
	}

	USCTDDeckRepository* DeckRepository = UserRepository->GetDeckRepository();
	if (!DeckRepository)
	{
		return FReply::Handled();
	}

	const FGuid DeckId = bIsEditingTurret ? EditingDeckId : GetOrCreatePrimaryDeckId();
	if (!DeckId.IsValid())
	{
		return FReply::Handled();
	}

	FSCTDPreparedTurretRecord TurretRecord;
	TurretRecord.InstanceId = bIsEditingTurret ? EditingTurretId : FGuid();
	TurretRecord.DisplayName = GetCurrentTurretName();
	TurretRecord.BasePartInstanceId = SelectedBasePart.InstanceId;
	TurretRecord.WeaponPartInstanceId = SelectedWeaponPart.InstanceId;
	TurretRecord.ControlPartInstanceId = SelectedControlPart.InstanceId;

	const bool bSaved = bIsEditingTurret
		? DeckRepository->UpdateTurretInDeck(DeckId, TurretRecord)
		: DeckRepository->AddTurretToDeck(DeckId, TurretRecord).IsValid();

	if (bSaved)
	{
		UserRepository->Save();
		ClearAssembly();
		RefreshOwnedTurretList();
		RefreshAssemblyPreview();
		RefreshAssemblyStats();
	}

	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandleEditTurretClicked(FGuid DeckId, FSCTDPreparedTurretRecord TurretRecord)
{
	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!PartsRepository)
	{
		return FReply::Handled();
	}

	bHasSelectedBasePart = PartsRepository->FindPart(TurretRecord.BasePartInstanceId, SelectedBasePart);
	bHasSelectedWeaponPart = PartsRepository->FindPart(TurretRecord.WeaponPartInstanceId, SelectedWeaponPart);
	bHasSelectedControlPart = PartsRepository->FindPart(TurretRecord.ControlPartInstanceId, SelectedControlPart);
	bIsEditingTurret = true;
	bIsViewingTurret = false;
	EditingDeckId = DeckId;
	EditingTurretId = TurretRecord.InstanceId;

	RefreshAssemblyPreview();
	RefreshAssemblyStats();
	if (TurretNameTextBox)
	{
		TurretNameTextBox->SetText(FText::FromString(TurretRecord.DisplayName));
	}
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandleDeleteTurretClicked(FGuid DeckId, FGuid TurretInstanceId)
{
	USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (DeckRepository && DeckRepository->RemoveTurretFromDeck(DeckId, TurretInstanceId))
	{
		UserRepository->Save();
		if ((bIsEditingTurret || bIsViewingTurret) && EditingTurretId == TurretInstanceId)
		{
			ClearAssembly();
			RefreshAssemblyPreview();
			RefreshAssemblyStats();
		}
		RefreshOwnedTurretList();
	}
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandleMoveTurretClicked(FGuid DeckId, FGuid TurretInstanceId, int32 Direction)
{
	USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (DeckRepository && DeckRepository->MoveTurretInDeck(DeckId, TurretInstanceId, Direction))
	{
		UserRepository->Save();
		RefreshOwnedTurretList();
	}
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandleLobbyClicked()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("Lobby")));
	return FReply::Handled();
}

void ULabTurretFusionWidget::RefreshAssemblyPreview()
{
	if (!PreviewContentBox)
	{
		return;
	}

	PreviewContentBox->ClearChildren();
	PreviewContentBox->AddSlot().AutoHeight()
	[
		SNew(STextBlock)
		.Text(FText::FromString(bIsEditingTurret ? TEXT("EDIT TURRET ASSEMBLY") : (bIsViewingTurret ? TEXT("VIEW TURRET BUILD") : TEXT("TURRET BUILD PREVIEW"))))
		.ColorAndOpacity(FLinearColor(0.00f, 0.76f, 0.25f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
	];
	PreviewContentBox->AddSlot().FillHeight(1.0f).VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("BODY    %s\nWEAPON  %s\nCONTROL %s"),
			*PartLabel(bHasSelectedBasePart, SelectedBasePart, TEXT("body")),
			*PartLabel(bHasSelectedWeaponPart, SelectedWeaponPart, TEXT("weapon")),
			*PartLabel(bHasSelectedControlPart, SelectedControlPart, TEXT("control")))))
		.ColorAndOpacity(FLinearColor(0.72f, 0.84f, 0.74f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
		.Justification(ETextJustify::Center)
	];

	if (IsAssemblyComplete() && !bIsViewingTurret)
	{
		const FString DefaultName = bHasSelectedWeaponPart ? SelectedWeaponPart.DisplayName : TEXT("New Turret");
		PreviewContentBox->AddSlot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SAssignNew(TurretNameTextBox, SEditableTextBox)
				.Text(FText::FromString(DefaultName))
				.HintText(FText::FromString(TEXT("Turret name")))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(18.0f, 7.0f))
				.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleRegisterTurretClicked))
				[
					SNew(STextBlock)
					.Text(FText::FromString(bIsEditingTurret ? TEXT("SAVE") : TEXT("REGISTER")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]
			]
		];
	}
	else
	{
		TurretNameTextBox.Reset();
	}
}

void ULabTurretFusionWidget::RefreshAssemblyStats()
{
	if (!StatsContentBox)
	{
		return;
	}

	const int32 BuildCost = (bHasSelectedBasePart ? SelectedBasePart.BuildCost : 0)
		+ (bHasSelectedWeaponPart ? SelectedWeaponPart.BuildCost : 0)
		+ (bHasSelectedControlPart ? SelectedControlPart.BuildCost : 0);
	const float BuildTime = (bHasSelectedBasePart ? SelectedBasePart.BuildTimeSeconds : 0.0f)
		+ (bHasSelectedWeaponPart ? SelectedWeaponPart.BuildTimeSeconds : 0.0f)
		+ (bHasSelectedControlPart ? SelectedControlPart.BuildTimeSeconds : 0.0f);
	const float Health = bHasSelectedBasePart ? SelectedBasePart.BaseHealth : 0.0f;
	const float Defense = bHasSelectedBasePart ? SelectedBasePart.Defense : 0.0f;
	const float Attack = bHasSelectedWeaponPart ? SelectedWeaponPart.AttackDamage : 0.0f;
	const float AttackSpeed = bHasSelectedWeaponPart ? SelectedWeaponPart.AttackSpeed : 0.0f;
	const FString Control = bHasSelectedControlPart ? SelectedControlPart.AIProfileId.ToString() : TEXT("None");

	StatsContentBox->ClearChildren();
	StatsContentBox->AddSlot().AutoHeight()
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("TURRET STATS")))
		.ColorAndOpacity(FLinearColor(0.00f, 0.58f, 0.82f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
	];
	StatsContentBox->AddSlot().FillHeight(1.0f).VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("COST %d    TIME %.1fs\nHP %.0f    DEF %.0f    ATK %.0f    SPD %.2f\nAI %s"),
			BuildCost, BuildTime, Health, Defense, Attack, AttackSpeed, *Control)))
		.ColorAndOpacity(FLinearColor(0.70f, 0.86f, 0.94f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		.Justification(ETextJustify::Center)
	];
}

void ULabTurretFusionWidget::RefreshOwnedTurretList()
{
	if (!OwnedTurretScrollBox)
	{
		return;
	}

	OwnedTurretScrollBox->ClearChildren();

	const USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository)
	{
		return;
	}

	const TArray<FSCTDTurretDeckRecord> Decks = DeckRepository->GetDecks();
	if (!Decks.IsValidIndex(SelectedDeckIndex))
	{
		if (CanAddNewTurret())
		{
			OwnedTurretScrollBox->AddSlot().Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				BuildPlusTurretItem()
			];
		}
		return;
	}

	const FSCTDTurretDeckRecord& DeckRecord = Decks[SelectedDeckIndex];
	for (int32 TurretIndex = 0; TurretIndex < DeckRecord.Turrets.Num(); ++TurretIndex)
	{
		OwnedTurretScrollBox->AddSlot().Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			BuildPreparedTurretItem(DeckRecord.DeckId, DeckRecord.Turrets[TurretIndex], TurretIndex, DeckRecord.Turrets.Num())
		];
	}

	if (DeckRecord.Turrets.Num() < USCTDDeckRepository::MaxTurretsPerDeck)
	{
		OwnedTurretScrollBox->AddSlot().Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			BuildPlusTurretItem()
		];
	}
}

void ULabTurretFusionWidget::RefreshPartsList()
{
	if (!PartsScrollBox)
	{
		return;
	}

	PartsScrollBox->ClearChildren();
	const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!PartsRepository)
	{
		return;
	}

	for (const FSCTDOwnedTurretPartRecord& PartRecord : PartsRepository->GetOwnedPartsByType(SelectedPartType))
	{
		PartsScrollBox->AddSlot().Padding(0.0f, 0.0f, 0.0f, 10.0f)[BuildPartItem(PartRecord)];
	}
}

void ULabTurretFusionWidget::StartNewAssembly()
{
	ClearAssembly();
	bIsViewingTurret = false;
	bIsEditingTurret = false;
	RefreshAssemblyPreview();
	RefreshAssemblyStats();
}

bool ULabTurretFusionWidget::IsAssemblyComplete() const
{
	return bHasSelectedBasePart && bHasSelectedWeaponPart && bHasSelectedControlPart;
}

bool ULabTurretFusionWidget::CanAddNewTurret() const
{
	const USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository)
	{
		return false;
	}

	const TArray<FSCTDTurretDeckRecord> Decks = DeckRepository->GetDecks();
	if (!Decks.IsValidIndex(SelectedDeckIndex))
	{
		return DeckRepository->CanCreateDeck();
	}

	return DeckRepository->CanAddTurretToDeck(Decks[SelectedDeckIndex].DeckId);
}

FGuid ULabTurretFusionWidget::GetOrCreatePrimaryDeckId()
{
	USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository)
	{
		return FGuid();
	}

	return GetOrCreateDeckIdByIndex(SelectedDeckIndex);
}

FGuid ULabTurretFusionWidget::GetSelectedDeckId() const
{
	const USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository)
	{
		return FGuid();
	}

	const TArray<FSCTDTurretDeckRecord> Decks = DeckRepository->GetDecks();
	return Decks.IsValidIndex(SelectedDeckIndex) ? Decks[SelectedDeckIndex].DeckId : FGuid();
}

FGuid ULabTurretFusionWidget::GetOrCreateDeckIdByIndex(int32 DeckIndex)
{
	USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository)
	{
		return FGuid();
	}

	DeckIndex = FMath::Clamp(DeckIndex, 0, USCTDDeckRepository::MaxDeckCount - 1);
	TArray<FSCTDTurretDeckRecord> Decks = DeckRepository->GetDecks();
	while (!Decks.IsValidIndex(DeckIndex) && DeckRepository->CanCreateDeck())
	{
		DeckRepository->CreateDeck(FString::Printf(TEXT("Deck %d"), Decks.Num() + 1));
		Decks = DeckRepository->GetDecks();
	}

	return Decks.IsValidIndex(DeckIndex) ? Decks[DeckIndex].DeckId : FGuid();
}

FString ULabTurretFusionWidget::GetCurrentTurretName() const
{
	FString Name = TurretNameTextBox ? TurretNameTextBox->GetText().ToString() : FString();
	Name.TrimStartAndEndInline();
	if (Name.IsEmpty())
	{
		Name = bHasSelectedWeaponPart ? SelectedWeaponPart.DisplayName : TEXT("New Turret");
	}
	return Name;
}

void ULabTurretFusionWidget::ClearAssembly()
{
	bHasSelectedBasePart = false;
	bHasSelectedWeaponPart = false;
	bHasSelectedControlPart = false;
	bIsEditingTurret = false;
	bIsViewingTurret = false;
	EditingDeckId.Invalidate();
	EditingTurretId.Invalidate();
	TurretNameTextBox.Reset();
}
