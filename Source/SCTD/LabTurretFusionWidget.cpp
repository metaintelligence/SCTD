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
#include "SCTDMarqueeText.h"

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

	bool HasDamageBonus(float Ratio)
	{
		return Ratio > KINDA_SMALL_NUMBER;
	}

	float SumOptionValue(const FSCTDOwnedTurretPartRecord& PartRecord, FName OptionId)
	{
		float Sum = 0.0f;
		for (const FSCTDTurretPartOption& Option : PartRecord.AdditionalOptions)
		{
			if (Option.OptionId == OptionId)
			{
				Sum += Option.Value;
			}
		}
		return Sum;
	}

	FString BuildFlatFormula(float CurrentValue, float Delta)
	{
		if (FMath::Abs(Delta) <= KINDA_SMALL_NUMBER)
		{
			return FString::Printf(TEXT("%.0f"), CurrentValue);
		}

		return FString::Printf(TEXT("%.0f (%.0f+%.0f)"), CurrentValue, CurrentValue - Delta, Delta);
	}

	FString BuildRatioFormula(float CurrentValue, float Ratio)
	{
		if (Ratio <= KINDA_SMALL_NUMBER)
		{
			return FString::Printf(TEXT("%.2f"), CurrentValue);
		}

		const float BaseValue = CurrentValue / (1.0f + Ratio);
		return FString::Printf(TEXT("%.2f (%.2f+%.2f)"), CurrentValue, BaseValue, CurrentValue - BaseValue);
	}

	FString FormatStatNumber(float Value, int32 DecimalPlaces)
	{
		return DecimalPlaces > 0
			? FString::Printf(TEXT("%.*f"), DecimalPlaces, Value)
			: FString::Printf(TEXT("%.0f"), Value);
	}

	struct FLabPartBaseline
	{
		bool bValid = false;
		float BaseHealth = 0.0f;
		float Defense = 0.0f;
		float SelfRepairPerSecond = 0.0f;
		int32 BuildCost = 0;
		float BuildTimeSeconds = 0.0f;
	};

	bool TryGetLabPartBaseline(const FSCTDOwnedTurretPartRecord& PartRecord, FLabPartBaseline& OutBaseline)
	{
		OutBaseline = FLabPartBaseline();
		if (PartRecord.DefinitionId == TEXT("part_body_pylon"))
		{
			OutBaseline = { true, 400.0f, 10.0f, 1.0f, 100, 5.0f };
		}
		else if (PartRecord.DefinitionId == TEXT("part_body_quirass"))
		{
			OutBaseline = { true, 600.0f, 20.0f, 2.0f, 100, 2.0f };
		}
		else if (PartRecord.DefinitionId == TEXT("part_body_gunstock"))
		{
			OutBaseline = { true, 200.0f, 10.0f, 1.0f, 100, 3.0f };
		}
		else if (PartRecord.DefinitionId == TEXT("part_weapon_mortar"))
		{
			OutBaseline.bValid = true;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 5.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("part_weapon_minigun"))
		{
			OutBaseline.bValid = true;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 3.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("part_weapon_axe"))
		{
			OutBaseline.bValid = true;
			OutBaseline.BuildCost = 100;
			OutBaseline.BuildTimeSeconds = 2.0f;
		}
		else if (PartRecord.DefinitionId.ToString().StartsWith(TEXT("part_control_")))
		{
			OutBaseline.bValid = true;
			OutBaseline.BuildCost = 0;
			OutBaseline.BuildTimeSeconds = 0.0f;
		}
		return OutBaseline.bValid;
	}

	int32 GetBaselineBuildCost(const FSCTDOwnedTurretPartRecord& PartRecord)
	{
		FLabPartBaseline Baseline;
		return TryGetLabPartBaseline(PartRecord, Baseline) ? Baseline.BuildCost : PartRecord.BuildCost;
	}

	float GetBaselineBuildTime(const FSCTDOwnedTurretPartRecord& PartRecord)
	{
		FLabPartBaseline Baseline;
		return TryGetLabPartBaseline(PartRecord, Baseline) ? Baseline.BuildTimeSeconds : PartRecord.BuildTimeSeconds;
	}
}

void ULabTurretFusionWidget::SetUserRepository(USCTDUserRepository* NewUserRepository)
{
	UserRepository = NewUserRepository;
	SelectedDeckIndex = UserRepository ? UserRepository->GetSelectedTurretDeckIndex() : 0;
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
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SAssignNew(HoverCardBox, SBox)
			.Visibility(EVisibility::Collapsed)
		];
}

void ULabTurretFusionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HoverCardBox || !HoveredPart.IsSet())
	{
		return;
	}

	HoverCardBox->SetRenderTransform(FSlateRenderTransform(HoverCardPosition));
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
				SNew(SSCTDMarqueeText)
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
					SNew(SSCTDMarqueeText)
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
					SNew(SSCTDMarqueeText)
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
			SNew(SSCTDMarqueeText)
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
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(TEXT("+")))
							.ColorAndOpacity(bCanAdd ? FLinearColor(1.0f, 0.34f, 0.34f, 1.0f) : FLinearColor(0.34f, 0.34f, 0.36f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
							.Justification(ETextJustify::Center)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SSCTDMarqueeText)
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
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(FString::Printf(TEXT("%d"), TurretIndex + 1)))
						.ColorAndOpacity(FLinearColor(1.0f, 0.38f, 0.38f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(TurretRecord.DisplayName))
							.ColorAndOpacity(FLinearColor(1.0f, 0.62f, 0.62f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(TEXT("REGISTERED BUILD")))
							.ColorAndOpacity(FLinearColor(0.72f, 0.38f, 0.38f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(116.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(54.0f)
									[
										SNew(SButton)
										.ContentPadding(FMargin(4.0f, 4.0f))
										.IsEnabled(TurretIndex > 0)
										.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleMoveTurretClicked, DeckId, TurretRecord.InstanceId, -1))
										[
											SNew(SSCTDMarqueeText)
											.Text(FText::FromString(TEXT("UP")))
											.Justification(ETextJustify::Center)
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										]
									]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SBox)
									.WidthOverride(54.0f)
									[
										SNew(SButton)
										.ContentPadding(FMargin(4.0f, 4.0f))
										.IsEnabled(TurretIndex < TurretCount - 1)
										.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleMoveTurretClicked, DeckId, TurretRecord.InstanceId, 1))
										[
											SNew(SSCTDMarqueeText)
											.Text(FText::FromString(TEXT("DN")))
											.Justification(ETextJustify::Center)
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										]
									]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(54.0f)
									[
										SNew(SButton)
										.ContentPadding(FMargin(4.0f, 4.0f))
										.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleEditTurretClicked, DeckId, TurretRecord))
										[
											SNew(SSCTDMarqueeText)
											.Text(FText::FromString(TEXT("EDIT")))
											.Justification(ETextJustify::Center)
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										]
									]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SBox)
									.WidthOverride(54.0f)
									[
										SNew(SButton)
										.ContentPadding(FMargin(4.0f, 4.0f))
										.ButtonColorAndOpacity(FLinearColor(0.55f, 0.06f, 0.06f, 1.0f))
										.OnClicked(FOnClicked::CreateUObject(this, &ULabTurretFusionWidget::HandleDeleteTurretClicked, DeckId, TurretRecord.InstanceId))
										[
											SNew(SSCTDMarqueeText)
											.Text(FText::FromString(TEXT("DEL")))
											.Justification(ETextJustify::Center)
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										]
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
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(FLinearColor(0.92f, 0.82f, 0.96f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildPartItem(const FSCTDOwnedTurretPartRecord& PartRecord)
{
	const FLinearColor TypeAccentColor = PartAccentColor(PartRecord.PartType);
	const FLinearColor RarityColor = PartRecord.RarityColor.A > KINDA_SMALL_NUMBER ? PartRecord.RarityColor : FLinearColor::White;
	FString StatLine;
	if (PartRecord.PartType == ESCTDTurretPartType::Base)
	{
		StatLine = FString::Printf(TEXT("%s / HP %.0f / DEF %.0f / REP %.1f"), *BuildMountTypeText(PartRecord.MountType), PartRecord.BaseHealth, PartRecord.Defense, PartRecord.SelfRepairPerSecond);
	}
	else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
	{
		StatLine = FString::Printf(TEXT("%s / ATK %.0f-%.0f / SPD %.2f / RNG %.0f / AOE %.0f"), *BuildMountTypeText(PartRecord.MountType), PartRecord.MinAttackDamage, PartRecord.MaxAttackDamage, PartRecord.AttackSpeed, PartRecord.AttackRange, PartRecord.AreaAttackRange);
	}
	else
	{
		StatLine = FString::Printf(TEXT("AI %s"), *StaticEnum<ESCTDTargetingAI>()->GetDisplayNameTextByValue(static_cast<int64>(PartRecord.TargetingAI)).ToString());
	}

	return SNew(SBox)
		.HeightOverride(88.0f)
		[
			SNew(SButton)
			.ButtonColorAndOpacity(FLinearColor::White)
			.ContentPadding(0.0f)
			.OnHovered(FSimpleDelegate::CreateUObject(this, &ULabTurretFusionWidget::HandlePartHovered, PartRecord))
			.OnUnhovered(FSimpleDelegate::CreateUObject(this, &ULabTurretFusionWidget::HandlePartUnhovered))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(RarityColor.CopyWithNewOpacity(0.50f))
				.Padding(1.0f)
				.OnMouseMove(FPointerEventHandler::CreateUObject(this, &ULabTurretFusionWidget::HandlePartMouseMove, PartRecord))
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
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(PartRecord.DisplayName))
							.ColorAndOpacity(RarityColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(StatLine))
							.ColorAndOpacity(TypeAccentColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(FString::Printf(TEXT("COST %d / TIME %.1fs"), PartRecord.BuildCost, PartRecord.BuildTimeSeconds)))
							.ColorAndOpacity(FLinearColor(0.58f, 0.52f, 0.62f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						]
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
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(Description))
				.ColorAndOpacity(AccentColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildStatsSectionTitle(const FString& Label) const
{
	return SNew(SSCTDMarqueeText)
		.Text(FText::FromString(Label))
		.ColorAndOpacity(FLinearColor(0.70f, 0.82f, 0.92f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		.Justification(ETextJustify::Left);
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildStatsRow(const FString& Label, const FString& Value, const FLinearColor& ValueColor) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(108.0f)
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(FLinearColor(0.62f, 0.66f, 0.72f, 1.0f))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(Value))
			.ColorAndOpacity(ValueColor)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			.Justification(ETextJustify::Left)
			.TextLengthHandling(ESCTDTextLengthHandling::RenderOverflow)
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildStatsFormulaRow(const FString& Label, float CurrentValue, float BaseValue, const FString& Suffix, int32 DecimalPlaces) const
{
	const FLinearColor BaseColor(0.82f, 0.86f, 0.96f, 1.0f);
	const FLinearColor ChangedColor(0.34f, 0.48f, 1.0f, 1.0f);
	const float Delta = CurrentValue - BaseValue;
	const bool bChanged = FMath::Abs(Delta) > 0.01f;
	const FString CurrentText = FormatStatNumber(CurrentValue, DecimalPlaces);
	const FString BaseText = FormatStatNumber(BaseValue, DecimalPlaces);
	const FString DeltaText = FormatStatNumber(FMath::Abs(Delta), DecimalPlaces);

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(108.0f)
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(FLinearColor(0.62f, 0.66f, 0.72f, 1.0f))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(CurrentText))
				.ColorAndOpacity(bChanged ? ChangedColor : BaseColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(bChanged ? TEXT(" (") : TEXT("")))
				.ColorAndOpacity(ChangedColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(bChanged ? BaseText : TEXT("")))
				.ColorAndOpacity(BaseColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(bChanged ? FString::Printf(TEXT("%s%s)"), Delta >= 0.0f ? TEXT("+") : TEXT("-"), *DeltaText) : TEXT("")))
				.ColorAndOpacity(ChangedColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSCTDMarqueeText)
				.Text(FText::FromString(Suffix))
				.ColorAndOpacity(BaseColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Left)
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildAttributeDamageRow(const FString& Label, float MinBaseDamage, float MaxBaseDamage, float Ratio) const
{
	const float ClampedRatio = FMath::Max(0.0f, Ratio);
	return BuildStatsRow(
		Label,
		FString::Printf(TEXT("+%.1f-%.1f (%.0f%%)"), MinBaseDamage * ClampedRatio, MaxBaseDamage * ClampedRatio, ClampedRatio * 100.0f),
		FLinearColor(0.34f, 0.48f, 1.0f, 1.0f));
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildItemViewerCard(const FSCTDOwnedTurretPartRecord& PartRecord) const
{
	TSharedRef<SVerticalBox> BasicStatsBox = SNew(SVerticalBox);
	if (PartRecord.PartType == ESCTDTurretPartType::Base)
	{
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Health: %.0f"), PartRecord.BaseHealth), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Defense: %.0f"), PartRecord.Defense), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Repair: %.1f / sec"), PartRecord.SelfRepairPerSecond), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Build: %d / %.1fs"), PartRecord.BuildCost, PartRecord.BuildTimeSeconds), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
	}
	else if (PartRecord.PartType == ESCTDTurretPartType::Weapon)
	{
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Damage: %.0f-%.0f"), PartRecord.MinAttackDamage, PartRecord.MaxAttackDamage), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Speed: %.2f / sec"), PartRecord.AttackSpeed), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Range: %.0f"), PartRecord.AttackRange), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Area: %.0f"), PartRecord.AreaAttackRange), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Crit: %.0f%% / x%.2f"), PartRecord.CriticalChance * 100.0f, PartRecord.CriticalDamageMultiplier), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
	}
	else
	{
		BasicStatsBox->AddSlot().AutoHeight()[BuildItemViewerLine(FString::Printf(TEXT("Targeting AI: %s"), *StaticEnum<ESCTDTargetingAI>()->GetDisplayNameTextByValue(static_cast<int64>(PartRecord.TargetingAI)).ToString()), FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
	}

	TSharedRef<SVerticalBox> OptionsBox = SNew(SVerticalBox);
	for (const FSCTDTurretPartOption& Option : PartRecord.AdditionalOptions)
	{
		OptionsBox->AddSlot().AutoHeight()
		[
			BuildItemViewerLine(FString::Printf(TEXT("%s %s"), *GetOptionLabel(Option.OptionId), *BuildOptionValueText(Option)), FLinearColor(0.34f, 0.48f, 1.0f, 1.0f))
		];
	}

	return SNew(SBox)
		.WidthOverride(360.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(PartRecord.RarityColor)
			.Padding(2.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.022f, 0.021f, 0.020f, 0.98f))
				.Padding(14.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(PartRecord.DisplayName))
						.ColorAndOpacity(PartRecord.RarityColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 10.0f)
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(FString::Printf(TEXT("%s / %s"), PartRecord.PartType == ESCTDTurretPartType::Base ? TEXT("BODY") : PartRecord.PartType == ESCTDTurretPartType::Weapon ? TEXT("WEAPON") : TEXT("CONTROL"), *BuildMountTypeText(PartRecord.MountType))))
						.ColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.68f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(76.0f)
							.HeightOverride(112.0f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
								.BorderBackgroundColor(FLinearColor(0.045f, 0.045f, 0.048f, 1.0f))
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(14.0f, 0.0f, 0.0f, 0.0f)
						[
							BasicStatsBox
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						OptionsBox
					]
				]
			]
		];
}

TSharedRef<SWidget> ULabTurretFusionWidget::BuildItemViewerLine(const FString& Text, const FLinearColor& Color, int32 FontSize) const
{
	return SNew(SSCTDMarqueeText)
		.Text(FText::FromString(Text))
		.ColorAndOpacity(Color)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", FontSize))
		.Justification(ETextJustify::Left)
		.TextLengthHandling(ESCTDTextLengthHandling::RenderOverflow);
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
		UserRepository->SetSelectedTurretDeckIndex(SelectedDeckIndex);
		UserRepository->Save();
	}

	ClearAssembly();
	RefreshOwnedTurretList();
	RefreshPartsList();
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
	const FGuid SelectedDeckId = GetSelectedDeckId();
	if (SelectedDeckId.IsValid() && IsPartUsedInDeck(SelectedDeckId, PartRecord.InstanceId))
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

	bHasSelectedBasePart = PartsRepository->FindResolvedPart(TurretRecord.BasePartInstanceId, SelectedBasePart);
	bHasSelectedWeaponPart = PartsRepository->FindResolvedPart(TurretRecord.WeaponPartInstanceId, SelectedWeaponPart);
	bHasSelectedControlPart = PartsRepository->FindResolvedPart(TurretRecord.ControlPartInstanceId, SelectedControlPart);
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
	if (!CanUseSelectedPartsInDeck(DeckId))
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
		RefreshPartsList();
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

	bHasSelectedBasePart = PartsRepository->FindResolvedPart(TurretRecord.BasePartInstanceId, SelectedBasePart);
	bHasSelectedWeaponPart = PartsRepository->FindResolvedPart(TurretRecord.WeaponPartInstanceId, SelectedWeaponPart);
	bHasSelectedControlPart = PartsRepository->FindResolvedPart(TurretRecord.ControlPartInstanceId, SelectedControlPart);
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
		RefreshPartsList();
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
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/Lobby")));
	return FReply::Handled();
}

FReply ULabTurretFusionWidget::HandlePartMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FSCTDOwnedTurretPartRecord PartRecord)
{
	constexpr float ViewerWidth = 360.0f;
	constexpr float ViewerGap = 8.0f;
	const FGeometry& RootGeometry = GetCachedGeometry();
	const FVector2D SlotPosition = RootGeometry.AbsoluteToLocal(MyGeometry.GetAbsolutePosition());
	HoverCardPosition = FVector2D(FMath::Max(0.0f, SlotPosition.X - ViewerWidth - ViewerGap), SlotPosition.Y);
	HandlePartHovered(PartRecord);
	return FReply::Unhandled();
}

void ULabTurretFusionWidget::HandlePartHovered(FSCTDOwnedTurretPartRecord PartRecord)
{
	HoveredPart = PartRecord;
	if (HoverCardBox)
	{
		HoverCardBox->SetContent(BuildItemViewerCard(PartRecord));
		HoverCardBox->SetVisibility(EVisibility::HitTestInvisible);
	}
}

void ULabTurretFusionWidget::HandlePartUnhovered()
{
	HoveredPart.Reset();
	if (HoverCardBox)
	{
		HoverCardBox->SetVisibility(EVisibility::Collapsed);
	}
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
		SNew(SSCTDMarqueeText)
		.Text(FText::FromString(bIsEditingTurret ? TEXT("EDIT TURRET ASSEMBLY") : (bIsViewingTurret ? TEXT("VIEW TURRET BUILD") : TEXT("TURRET BUILD PREVIEW"))))
		.ColorAndOpacity(FLinearColor(0.00f, 0.76f, 0.25f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
	];
	PreviewContentBox->AddSlot().FillHeight(1.0f).VAlign(VAlign_Center)
	[
		SNew(SSCTDMarqueeText)
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
				.IsEnabled(IsMountTypeMatched())
				.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleRegisterTurretClicked))
				[
					SNew(SSCTDMarqueeText)
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
	const int32 BaseBuildCost = (bHasSelectedBasePart ? GetBaselineBuildCost(SelectedBasePart) : 0)
		+ (bHasSelectedWeaponPart ? GetBaselineBuildCost(SelectedWeaponPart) : 0)
		+ (bHasSelectedControlPart ? GetBaselineBuildCost(SelectedControlPart) : 0);
	const float BuildTime = (bHasSelectedBasePart ? SelectedBasePart.BuildTimeSeconds : 0.0f)
		+ (bHasSelectedWeaponPart ? SelectedWeaponPart.BuildTimeSeconds : 0.0f)
		+ (bHasSelectedControlPart ? SelectedControlPart.BuildTimeSeconds : 0.0f);
	const float BaseBuildTime = (bHasSelectedBasePart ? GetBaselineBuildTime(SelectedBasePart) : 0.0f)
		+ (bHasSelectedWeaponPart ? GetBaselineBuildTime(SelectedWeaponPart) : 0.0f)
		+ (bHasSelectedControlPart ? GetBaselineBuildTime(SelectedControlPart) : 0.0f);
	const float Health = bHasSelectedBasePart ? SelectedBasePart.BaseHealth : 0.0f;
	const float Defense = bHasSelectedBasePart ? SelectedBasePart.Defense : 0.0f;
	const float SelfRepair = bHasSelectedBasePart ? SelectedBasePart.SelfRepairPerSecond : 0.0f;
	FLabPartBaseline BasePartBaseline;
	const bool bHasBasePartBaseline = bHasSelectedBasePart && TryGetLabPartBaseline(SelectedBasePart, BasePartBaseline);
	const float MinAttack = bHasSelectedWeaponPart ? SelectedWeaponPart.MinAttackDamage : 0.0f;
	const float MaxAttack = bHasSelectedWeaponPart ? SelectedWeaponPart.MaxAttackDamage : 0.0f;
	const float WeaponAttackSpeed = bHasSelectedWeaponPart ? SelectedWeaponPart.AttackSpeed : 0.0f;
	const float WeaponAttackSpeedOptionRatio = bHasSelectedWeaponPart ? FMath::Max(0.0f, SumOptionValue(SelectedWeaponPart, TEXT("IncreaseAttackSpeed"))) : 0.0f;
	const float WeaponBaseAttackSpeed = WeaponAttackSpeedOptionRatio > KINDA_SMALL_NUMBER ? WeaponAttackSpeed / (1.0f + WeaponAttackSpeedOptionRatio) : WeaponAttackSpeed;
	const float ExternalAttackSpeedBonusRatio = (bHasSelectedBasePart ? FMath::Max(0.0f, SelectedBasePart.AttackSpeed) : 0.0f)
		+ (bHasSelectedControlPart ? FMath::Max(0.0f, SelectedControlPart.AttackSpeed) : 0.0f);
	const float AttackSpeed = WeaponAttackSpeed * (1.0f + FMath::Max(0.0f, ExternalAttackSpeedBonusRatio));
	const float AttackRange = bHasSelectedWeaponPart ? SelectedWeaponPart.AttackRange : 0.0f;
	const float AreaRange = bHasSelectedWeaponPart ? SelectedWeaponPart.AreaAttackRange : 0.0f;
	const float AttackRangeBonus = bHasSelectedWeaponPart ? SumOptionValue(SelectedWeaponPart, TEXT("IncreaseAttackRange")) : 0.0f;
	const float AreaRangeBonus = bHasSelectedWeaponPart ? SumOptionValue(SelectedWeaponPart, TEXT("IncreaseAreaRange")) : 0.0f;
	const float CriticalChance = FMath::Clamp((bHasSelectedWeaponPart ? SelectedWeaponPart.CriticalChance : 0.0f)
		+ (bHasSelectedBasePart ? SelectedBasePart.CriticalChance : 0.0f)
		+ (bHasSelectedControlPart ? SelectedControlPart.CriticalChance : 0.0f), 0.0f, 1.0f);
	const float CriticalDamage = (bHasSelectedWeaponPart ? SelectedWeaponPart.CriticalDamageMultiplier : 1.5f)
		+ (bHasSelectedBasePart ? FMath::Max(0.0f, SelectedBasePart.CriticalDamageMultiplier - 1.5f) : 0.0f)
		+ (bHasSelectedControlPart ? FMath::Max(0.0f, SelectedControlPart.CriticalDamageMultiplier - 1.5f) : 0.0f);
	const float PhysicalDamageBonusRatio = (bHasSelectedBasePart ? SelectedBasePart.PhysicalDamageBonusRatio : 0.0f) + (bHasSelectedWeaponPart ? SelectedWeaponPart.PhysicalDamageBonusRatio : 0.0f) + (bHasSelectedControlPart ? SelectedControlPart.PhysicalDamageBonusRatio : 0.0f);
	const float FireDamageBonusRatio = (bHasSelectedBasePart ? SelectedBasePart.FireDamageBonusRatio : 0.0f) + (bHasSelectedWeaponPart ? SelectedWeaponPart.FireDamageBonusRatio : 0.0f) + (bHasSelectedControlPart ? SelectedControlPart.FireDamageBonusRatio : 0.0f);
	const float LightningDamageBonusRatio = (bHasSelectedBasePart ? SelectedBasePart.LightningDamageBonusRatio : 0.0f) + (bHasSelectedWeaponPart ? SelectedWeaponPart.LightningDamageBonusRatio : 0.0f) + (bHasSelectedControlPart ? SelectedControlPart.LightningDamageBonusRatio : 0.0f);
	const float FrostDamageBonusRatio = (bHasSelectedBasePart ? SelectedBasePart.FrostDamageBonusRatio : 0.0f) + (bHasSelectedWeaponPart ? SelectedWeaponPart.FrostDamageBonusRatio : 0.0f) + (bHasSelectedControlPart ? SelectedControlPart.FrostDamageBonusRatio : 0.0f);
	const float TotalDamageBonusRatio = FMath::Max(0.0f, PhysicalDamageBonusRatio)
		+ FMath::Max(0.0f, FireDamageBonusRatio)
		+ FMath::Max(0.0f, LightningDamageBonusRatio)
		+ FMath::Max(0.0f, FrostDamageBonusRatio);
	const FString Control = bHasSelectedControlPart
		? StaticEnum<ESCTDTargetingAI>()->GetDisplayNameTextByValue(static_cast<int64>(SelectedControlPart.TargetingAI)).ToString()
		: TEXT("None");
	const FString AttackAttributeText = bHasSelectedWeaponPart ? BuildAttackAttributeText(SelectedWeaponPart.AttackAttribute) : TEXT("None");
	const bool bHasAnyAttributeDamage = HasDamageBonus(PhysicalDamageBonusRatio)
		|| HasDamageBonus(FireDamageBonusRatio)
		|| HasDamageBonus(LightningDamageBonusRatio)
		|| HasDamageBonus(FrostDamageBonusRatio);

	TSharedRef<SVerticalBox> WeaponStatsBox = SNew(SVerticalBox);
	const FString TotalDamageText = TotalDamageBonusRatio > KINDA_SMALL_NUMBER
		? FString::Printf(TEXT("%.1f-%.1f"), MinAttack * (1.0f + TotalDamageBonusRatio), MaxAttack * (1.0f + TotalDamageBonusRatio))
		: FString::Printf(TEXT("%.0f-%.0f"), MinAttack, MaxAttack);
	WeaponStatsBox->AddSlot().AutoHeight()[BuildStatsSectionTitle(TEXT("WEAPON"))];
	WeaponStatsBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[BuildStatsRow(TEXT("Total Damage"), TotalDamageText, TotalDamageBonusRatio > 0.0f ? FLinearColor(0.34f, 0.48f, 1.0f, 1.0f) : FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))];
	WeaponStatsBox->AddSlot().AutoHeight()[BuildStatsRow(TEXT("Attribute"), AttackAttributeText)];
	WeaponStatsBox->AddSlot().AutoHeight()[BuildStatsFormulaRow(TEXT("Speed"), AttackSpeed, WeaponBaseAttackSpeed, TEXT(" / sec"), 2)];
	WeaponStatsBox->AddSlot().AutoHeight()[BuildStatsFormulaRow(TEXT("Range"), AttackRange, AttackRange - AttackRangeBonus)];
	WeaponStatsBox->AddSlot().AutoHeight()[BuildStatsFormulaRow(TEXT("Area"), AreaRange, AreaRange - AreaRangeBonus)];
	WeaponStatsBox->AddSlot().AutoHeight()[BuildStatsRow(TEXT("Crit"), FString::Printf(TEXT("%.0f%% / x%.2f"), CriticalChance * 100.0f, CriticalDamage))];
	if (bHasAnyAttributeDamage)
	{
		WeaponStatsBox->AddSlot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[BuildStatsSectionTitle(TEXT("ADDITIONAL ATTRIBUTE DAMAGE"))];
		WeaponStatsBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[BuildStatsRow(TEXT("Base DMG"), FString::Printf(TEXT("%.0f~%.0f"), MinAttack, MaxAttack))];
		if (HasDamageBonus(PhysicalDamageBonusRatio))
		{
			WeaponStatsBox->AddSlot().AutoHeight()[BuildAttributeDamageRow(TEXT("Physical"), MinAttack, MaxAttack, PhysicalDamageBonusRatio)];
		}
		if (HasDamageBonus(FireDamageBonusRatio))
		{
			WeaponStatsBox->AddSlot().AutoHeight()[BuildAttributeDamageRow(TEXT("Fire"), MinAttack, MaxAttack, FireDamageBonusRatio)];
		}
		if (HasDamageBonus(LightningDamageBonusRatio))
		{
			WeaponStatsBox->AddSlot().AutoHeight()[BuildAttributeDamageRow(TEXT("Lightning"), MinAttack, MaxAttack, LightningDamageBonusRatio)];
		}
		if (HasDamageBonus(FrostDamageBonusRatio))
		{
			WeaponStatsBox->AddSlot().AutoHeight()[BuildAttributeDamageRow(TEXT("Frost"), MinAttack, MaxAttack, FrostDamageBonusRatio)];
		}
	}

	StatsContentBox->ClearChildren();
	StatsContentBox->AddSlot().AutoHeight()
	[
		SNew(SSCTDMarqueeText)
		.Text(FText::FromString(TEXT("TURRET DETAIL VIEWER")))
		.ColorAndOpacity(FLinearColor(0.00f, 0.58f, 0.82f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
		.Justification(ETextJustify::Left)
	];
	StatsContentBox->AddSlot().FillHeight(1.0f).Padding(0.0f, 8.0f, 0.0f, 0.0f)
	[
		SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Collapsed)
		+ SScrollBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 18.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[BuildStatsSectionTitle(TEXT("ASSEMBLY"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[BuildStatsRow(TEXT("Body Part"), PartLabel(bHasSelectedBasePart, SelectedBasePart, TEXT("body")), bHasSelectedBasePart ? SelectedBasePart.RarityColor : FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))]
				+ SVerticalBox::Slot().AutoHeight()[BuildStatsRow(TEXT("Weapon Part"), PartLabel(bHasSelectedWeaponPart, SelectedWeaponPart, TEXT("weapon")), bHasSelectedWeaponPart ? SelectedWeaponPart.RarityColor : FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))]
				+ SVerticalBox::Slot().AutoHeight()[BuildStatsRow(TEXT("Control Part"), PartLabel(bHasSelectedControlPart, SelectedControlPart, TEXT("control")), bHasSelectedControlPart ? SelectedControlPart.RarityColor : FLinearColor(0.82f, 0.86f, 0.96f, 1.0f))]
				+ SVerticalBox::Slot().AutoHeight()[BuildStatsFormulaRow(TEXT("Build Cost"), static_cast<float>(BuildCost), static_cast<float>(BaseBuildCost))]
				+ SVerticalBox::Slot().AutoHeight()[BuildStatsFormulaRow(TEXT("Build Time"), BuildTime, BaseBuildTime, TEXT("s"), 1)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[BuildStatsSectionTitle(TEXT("BODY"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[BuildStatsFormulaRow(TEXT("Health"), Health, bHasBasePartBaseline ? BasePartBaseline.BaseHealth : Health)]
				+ SVerticalBox::Slot().AutoHeight()[BuildStatsFormulaRow(TEXT("Defense"), Defense, bHasBasePartBaseline ? BasePartBaseline.Defense : Defense)]
				+ SVerticalBox::Slot().AutoHeight()[BuildStatsFormulaRow(TEXT("Repair"), SelfRepair, bHasBasePartBaseline ? BasePartBaseline.SelfRepairPerSecond : SelfRepair, TEXT(" / sec"), 1)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)[BuildStatsSectionTitle(TEXT("CONTROL"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[BuildStatsRow(TEXT("Targeting AI"), Control)]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				WeaponStatsBox
			]
		]
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
		const FGuid SelectedDeckId = GetSelectedDeckId();
		if (SelectedDeckId.IsValid() && IsPartUsedInDeck(SelectedDeckId, PartRecord.InstanceId))
		{
			continue;
		}
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
	return bHasSelectedBasePart && bHasSelectedWeaponPart && bHasSelectedControlPart && IsMountTypeMatched();
}

bool ULabTurretFusionWidget::IsMountTypeMatched() const
{
	return !bHasSelectedBasePart || !bHasSelectedWeaponPart || SelectedBasePart.MountType == SelectedWeaponPart.MountType;
}

bool ULabTurretFusionWidget::IsPartUsedInDeck(const FGuid& DeckId, const FGuid& PartInstanceId, const FGuid& IgnoredTurretInstanceId) const
{
	const USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository || !DeckId.IsValid() || !PartInstanceId.IsValid())
	{
		return false;
	}

	FSCTDTurretDeckRecord DeckRecord;
	if (!DeckRepository->FindDeck(DeckId, DeckRecord))
	{
		return false;
	}

	for (const FSCTDPreparedTurretRecord& TurretRecord : DeckRecord.Turrets)
	{
		if (IgnoredTurretInstanceId.IsValid() && TurretRecord.InstanceId == IgnoredTurretInstanceId)
		{
			continue;
		}
		if (TurretRecord.BasePartInstanceId == PartInstanceId
			|| TurretRecord.WeaponPartInstanceId == PartInstanceId
			|| TurretRecord.ControlPartInstanceId == PartInstanceId)
		{
			return true;
		}
	}

	return false;
}

bool ULabTurretFusionWidget::CanUseSelectedPartsInDeck(const FGuid& DeckId) const
{
	const FGuid IgnoredTurretInstanceId = bIsEditingTurret ? EditingTurretId : FGuid();
	return (!bHasSelectedBasePart || !IsPartUsedInDeck(DeckId, SelectedBasePart.InstanceId, IgnoredTurretInstanceId))
		&& (!bHasSelectedWeaponPart || !IsPartUsedInDeck(DeckId, SelectedWeaponPart.InstanceId, IgnoredTurretInstanceId))
		&& (!bHasSelectedControlPart || !IsPartUsedInDeck(DeckId, SelectedControlPart.InstanceId, IgnoredTurretInstanceId));
}

FString ULabTurretFusionWidget::BuildMountTypeText(ESCTDTurretMountType MountType) const
{
	switch (MountType)
	{
	case ESCTDTurretMountType::Tower:
		return TEXT("TOWER");
	case ESCTDTurretMountType::Cannon:
		return TEXT("CANNON");
	case ESCTDTurretMountType::Arm:
		return TEXT("ARM");
	default:
		return TEXT("-");
	}
}

FString ULabTurretFusionWidget::BuildAttackAttributeText(ESCTDAttackAttribute AttackAttribute) const
{
	switch (AttackAttribute)
	{
	case ESCTDAttackAttribute::Physical:
		return TEXT("PHYSICAL");
	case ESCTDAttackAttribute::Fire:
		return TEXT("FIRE");
	case ESCTDAttackAttribute::Lightning:
		return TEXT("LIGHTNING");
	case ESCTDAttackAttribute::Frost:
		return TEXT("FROST");
	default:
		return TEXT("-");
	}
}

FString ULabTurretFusionWidget::BuildOptionValueText(const FSCTDTurretPartOption& Option) const
{
	if (FMath::Abs(Option.Value) <= 1.0f)
	{
		return FString::Printf(TEXT("+%.0f%%"), Option.Value * 100.0f);
	}
	return FString::Printf(TEXT("+%.1f"), Option.Value);
}

FString ULabTurretFusionWidget::GetOptionLabel(FName OptionId) const
{
	return OptionId.IsNone() ? TEXT("-") : OptionId.ToString();
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
