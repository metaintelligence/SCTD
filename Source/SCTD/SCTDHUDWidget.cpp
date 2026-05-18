#include "SCTDHUDWidget.h"

#include "DefenseManager.h"
#include "FlyingPlayerPawn.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Input/Reply.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Model/Repository/SCTDDeckRepository.h"
#include "Model/Repository/SCTDPartsRepository.h"
#include "Model/Repository/SCTDUserRepository.h"
#include "StatusComponent.h"
#include "Styling/CoreStyle.h"
#include "TurretStatsPopupWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "SCTDMarqueeText.h"

namespace
{
struct FSCTDResultTableColumn
{
	FString Header;
	float Width = 100.0f;
	ETextJustify::Type Justification = ETextJustify::Left;
};

TSharedRef<SWidget> BuildSCTDResultTable(
	const TArray<FSCTDResultTableColumn>& Columns,
	int32 RowCount,
	TFunction<FString(int32, int32)> CellTextGetter,
	TFunction<EVisibility(int32)> RowVisibilityGetter,
	TFunction<bool(int32)> RowHighlightGetter)
{
	const FLinearColor BorderColor(0.22f, 0.48f, 0.68f, 0.95f);
	const FLinearColor HeaderBackgroundColor(0.04f, 0.12f, 0.18f, 1.0f);
	const FLinearColor RowBackgroundColor(0.012f, 0.018f, 0.026f, 0.92f);
	const FLinearColor HeaderTextColor(0.90f, 0.96f, 1.0f, 1.0f);
	const FLinearColor RowTextColor(0.78f, 0.90f, 1.0f, 1.0f);
	const FLinearColor HighlightTextColor(1.0f, 0.86f, 0.32f, 1.0f);

	TSharedRef<SGridPanel> Grid = SNew(SGridPanel);

	auto AddCell = [&Grid, &Columns, BorderColor, HeaderBackgroundColor, RowBackgroundColor, HeaderTextColor, RowTextColor, HighlightTextColor, CellTextGetter, RowVisibilityGetter, RowHighlightGetter](
		int32 ColumnIndex,
		int32 RowIndex,
		const FString& Text,
		bool bHeader)
	{
		const FSCTDResultTableColumn& Column = Columns[ColumnIndex];
		const FLinearColor CellBackgroundColor = bHeader ? HeaderBackgroundColor : RowBackgroundColor;

		Grid->AddSlot(ColumnIndex, RowIndex)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(BorderColor)
			.Padding(1.0f)
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([bHeader, RowIndex, RowVisibilityGetter]()
			{
				return bHeader ? EVisibility::Visible : RowVisibilityGetter(RowIndex - 1);
			})))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(CellBackgroundColor)
				.Padding(FMargin(8.0f, 4.0f))
				[
					SNew(SBox)
					.WidthOverride(Column.Width)
					[
						SNew(SSCTDMarqueeText)
						.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([bHeader, Text, ColumnIndex, RowIndex, CellTextGetter]()
						{
							return FText::FromString(bHeader ? Text : CellTextGetter(RowIndex - 1, ColumnIndex));
						})))
						.ColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([bHeader, RowIndex, HeaderTextColor, RowTextColor, HighlightTextColor, RowHighlightGetter]()
						{
							if (bHeader)
							{
								return FSlateColor(HeaderTextColor);
							}
							return FSlateColor(RowHighlightGetter(RowIndex - 1) ? HighlightTextColor : RowTextColor);
						})))
						.Font(FCoreStyle::GetDefaultFontStyle("Mono", 11))
						.Justification(Column.Justification)
					]
				]
			]
		];
	};

	for (int32 ColumnIndex = 0; ColumnIndex < Columns.Num(); ++ColumnIndex)
	{
		AddCell(ColumnIndex, 0, Columns[ColumnIndex].Header, true);
	}

	for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns.Num(); ++ColumnIndex)
		{
			AddCell(ColumnIndex, RowIndex + 1, TEXT(""), false);
		}
	}

	return Grid;
}

TSharedRef<SWidget> BuildSCTDResultTableSection(const FString& Title, TSharedRef<SWidget> TableWidget)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SSCTDMarqueeText)
			.Text(FText::FromString(Title))
			.ColorAndOpacity(FLinearColor(1.0f, 0.84f, 0.36f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			TableWidget
		];
}
}

void USCTDHUDWidget::SetObservedPawn(APawn* NewObservedPawn)
{
	ObservedPawn = NewObservedPawn;
	ObservedStatus = NewObservedPawn ? NewObservedPawn->FindComponentByClass<UStatusComponent>() : nullptr;
	RefreshValues();
}

TSharedRef<SWidget> USCTDHUDWidget::RebuildWidget()
{
	HealthBarStyle = FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>(TEXT("ProgressBar"));
	HealthBarStyle.SetBackgroundImage(FSlateNoResource());
	HealthBarStyle.SetFillImage(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
	HealthBarStyle.SetMarqueeImage(FSlateNoResource());

	BoostBarStyle = HealthBarStyle;

	const FLinearColor PanelColor(0.005f, 0.018f, 0.022f, 0.82f);
	const FLinearColor FrameColor(0.00f, 0.92f, 0.84f, 0.70f);
	const FLinearColor TextColor(0.78f, 1.00f, 0.94f, 1.00f);
	const FLinearColor DimTextColor(0.38f, 0.78f, 0.78f, 1.00f);
	const float LeftHudWidth = 420.0f;
	const float SharedHudHeight = 123.0f;

	TSharedRef<SWidget> BuiltWidget =
		SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(28.0f, 0.0f, 0.0f, 28.0f))
		[
			SNew(SBox)
			.WidthOverride(LeftHudWidth)
			.HeightOverride(SharedHudHeight)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FrameColor)
				.Padding(FMargin(1.0f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor(PanelColor)
					.Padding(FMargin(16.0f, 14.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SAssignNew(ModeText, SSCTDMarqueeText)
								.Text(BuildModeText())
								.ColorAndOpacity(TextColor)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SAssignNew(TargetText, SSCTDMarqueeText)
								.Text(FText::FromString(TEXT("SCTD-01")))
								.ColorAndOpacity(DimTextColor)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SAssignNew(HealthText, SSCTDMarqueeText)
							.Text(BuildHealthText())
							.ColorAndOpacity(TextColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f, 0.0f, 0.0f)
						[
							SNew(SBox)
							.HeightOverride(16.0f)
							[
								SAssignNew(HealthBar, SProgressBar)
								.Style(&HealthBarStyle)
								.FillColorAndOpacity(FLinearColor(1.0f, 0.16f, 0.10f, 1.0f))
								.Percent(0.0f)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SAssignNew(BoostText, SSCTDMarqueeText)
							.Text(BuildBoostText())
							.ColorAndOpacity(TextColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f, 0.0f, 0.0f)
						[
							SNew(SBox)
							.HeightOverride(12.0f)
							[
								SAssignNew(BoostBar, SProgressBar)
								.Style(&BoostBarStyle)
								.FillColorAndOpacity(FLinearColor(0.0f, 0.90f, 1.0f, 1.0f))
								.Percent(0.0f)
							]
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(464.0f, 0.0f, 28.0f, 28.0f))
		[
			BuildTurretList()
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 28.0f, 28.0f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(390.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FrameColor)
				.Padding(FMargin(1.0f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor(PanelColor)
					.Padding(FMargin(12.0f, 10.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SAssignNew(ScrapText, SSCTDMarqueeText)
								.Text(BuildScrapText())
								.ColorAndOpacity(TextColor)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SAssignNew(DefenseTimeText, SSCTDMarqueeText)
								.Text(BuildDefenseTimeText())
								.ColorAndOpacity(DimTextColor)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 7.0f, 0.0f, 0.0f)
						[
							SAssignNew(MonsterCountText, SSCTDMarqueeText)
							.Text(BuildMonsterCountText())
							.ColorAndOpacity(TextColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
							.Justification(ETextJustify::Right)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f, 0.0f, 0.0f)
						[
							SAssignNew(LevelText, SSCTDMarqueeText)
							.Text(BuildLevelText())
							.ColorAndOpacity(DimTextColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.Justification(ETextJustify::Right)
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			BuildLevelUpChoiceOverlay()
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			BuildDefenseResultOverlay()
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SAssignNew(ConstructionGaugeBox, SBox)
			.WidthOverride(150.0f)
			.HeightOverride(34.0f)
			.Visibility(EVisibility::Collapsed)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ConstructionText, SSCTDMarqueeText)
					.Text(BuildConstructionText())
					.ColorAndOpacity(TextColor)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.HeightOverride(14.0f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
						.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.86f))
						.Padding(FMargin(2.0f))
						[
							SAssignNew(ConstructionBar, SProgressBar)
							.Style(&BoostBarStyle)
							.FillColorAndOpacity(FLinearColor(1.0f, 0.75f, 0.18f, 1.0f))
							.Percent(0.0f)
						]
					]
				]
			]
		];

	RefreshValues();
	return BuiltWidget;
}

void USCTDHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshObservedPawn();
	RefreshDefenseManager();
	RefreshValues();
	RefreshBuildListLayout();
	RefreshConstructionGaugeLocation();
	TickBuildListInertia(InDeltaTime);
	RefreshBuildTurretStatsPopupHover();
}

void USCTDHUDWidget::RefreshObservedPawn()
{
	if (ObservedPawn.IsValid() && ObservedStatus.IsValid())
	{
		return;
	}

	const APlayerController* PlayerController = GetOwningPlayer();
	SetObservedPawn(PlayerController ? PlayerController->GetPawn() : nullptr);
}

void USCTDHUDWidget::RefreshDefenseManager()
{
	if (DefenseManager.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ADefenseManager> It(World); It; ++It)
	{
		DefenseManager = *It;
		return;
	}
}

void USCTDHUDWidget::RefreshValues()
{
	const UStatusComponent* StatusComponent = ObservedStatus.Get();
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();

	if (HealthBar)
	{
		HealthBar->SetPercent(StatusComponent ? StatusComponent->GetHealthRatio() : 0.0f);
	}
	if (BoostBar)
	{
		BoostBar->SetPercent(StatusComponent ? StatusComponent->GetBoostRatio() : 0.0f);
		BoostBar->SetVisibility(StatusComponent && StatusComponent->UsesBoost() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
	if (HealthText)
	{
		HealthText->SetText(BuildHealthText());
	}
	if (BoostText)
	{
		BoostText->SetText(BuildBoostText());
		BoostText->SetVisibility(StatusComponent && StatusComponent->UsesBoost() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
	if (ModeText)
	{
		ModeText->SetText(BuildModeText());
	}
	if (ScrapText)
	{
		ScrapText->SetText(BuildScrapText());
	}
	if (DefenseTimeText)
	{
		DefenseTimeText->SetText(BuildDefenseTimeText());
	}
	if (MonsterCountText)
	{
		MonsterCountText->SetText(BuildMonsterCountText());
	}
	if (LevelText)
	{
		LevelText->SetText(BuildLevelText());
	}
	const bool bShowConstruction = CurrentDefenseManager && CurrentDefenseManager->IsConstructionActive();
	if (ConstructionText)
	{
		ConstructionText->SetText(BuildConstructionText());
		ConstructionText->SetVisibility(bShowConstruction ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
	if (ConstructionBar)
	{
		ConstructionBar->SetPercent(CurrentDefenseManager ? CurrentDefenseManager->GetConstructionProgressRatio() : 0.0f);
		ConstructionBar->SetVisibility(bShowConstruction ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
	if (ConstructionGaugeBox)
	{
		ConstructionGaugeBox->SetVisibility(bShowConstruction ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}
}

void USCTDHUDWidget::RefreshConstructionGaugeLocation()
{
	if (!ConstructionGaugeBox || !DefenseManager.IsValid() || !DefenseManager->IsConstructionActive())
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	FVector WorldLocation;
	if (!DefenseManager->GetConstructionWorldLocation(WorldLocation))
	{
		return;
	}

	FVector2D ScreenPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, ScreenPosition, false))
	{
		ConstructionGaugeBox->SetVisibility(EVisibility::Collapsed);
		return;
	}

	ScreenPosition -= FVector2D(75.0f, 17.0f);
	ConstructionGaugeBox->SetRenderTransform(FSlateRenderTransform(ScreenPosition));
}

void USCTDHUDWidget::RefreshBuildListLayout()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !BuildListBox)
	{
		return;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return;
	}

	const float ListHeight = 123.0f;
	const float LeftHudWidth = 420.0f;
	const float OuterPadding = 28.0f;
	const float GapFromLeftHud = 16.0f;
	const float AvailableWidth = ViewportWidth - LeftHudWidth - (OuterPadding * 2.0f) - GapFromLeftHud;
	const float ListWidth = FMath::Clamp(AvailableWidth, 360.0f, 760.0f);
	BuildListBox->SetHeightOverride(ListHeight);
	BuildListBox->SetWidthOverride(ListWidth);

	const float CardHeight = FMath::Max(44.0f, ListHeight - 18.0f);
	const float CardWidth = FMath::Clamp(CardHeight * 1.34f, 88.0f, 144.0f);
	BuildCardWidth = CardWidth;
	for (const TSharedPtr<SBox>& ItemBox : BuildItemBoxes)
	{
		if (ItemBox)
		{
			ItemBox->SetWidthOverride(CardWidth);
			ItemBox->SetHeightOverride(CardHeight);
		}
	}
}

void USCTDHUDWidget::LoadPreparedTurretsFromSelectedDeck()
{
	PreparedTurrets.Reset();

	UserRepository = USCTDUserRepository::CreateUserRepository(this);
	const USCTDDeckRepository* DeckRepository = UserRepository ? UserRepository->GetDeckRepository() : nullptr;
	if (!DeckRepository)
	{
		return;
	}

	const TArray<FSCTDTurretDeckRecord> Decks = DeckRepository->GetDecks();
	const int32 SelectedDeckIndex = UserRepository->GetSelectedTurretDeckIndex();
	if (Decks.IsValidIndex(SelectedDeckIndex))
	{
		PreparedTurrets = Decks[SelectedDeckIndex].Turrets;
	}
}

TSharedRef<SWidget> USCTDHUDWidget::BuildTurretList()
{
	BuildItemBoxes.Reset();
	LoadPreparedTurretsFromSelectedDeck();

	const FLinearColor PanelColor(0.006f, 0.012f, 0.014f, 0.76f);
	const FLinearColor FrameColor(0.0f, 0.92f, 0.84f, 0.62f);
	const FLinearColor AccentColors[] =
	{
		FLinearColor(0.0f, 0.92f, 0.84f, 1.0f),
		FLinearColor(1.0f, 0.50f, 0.18f, 1.0f),
		FLinearColor(0.28f, 0.72f, 1.0f, 1.0f),
		FLinearColor(0.95f, 0.92f, 0.36f, 1.0f),
		FLinearColor(0.42f, 0.55f, 1.0f, 1.0f),
		FLinearColor(0.66f, 1.0f, 0.30f, 1.0f),
		FLinearColor(1.0f, 0.30f, 0.62f, 1.0f)
	};

	TSharedRef<SScrollBox> ScrollBox = SAssignNew(BuildScrollBox, SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.AllowOverscroll(EAllowOverscroll::Yes);

	if (PreparedTurrets.IsEmpty())
	{
		ScrollBox->AddSlot()
		.Padding(FMargin(5.0f, 0.0f))
		[
			BuildTurretCard(TEXT("EMPTY"), TEXT("LAB REQUIRED"), FLinearColor(0.24f, 0.38f, 0.40f, 1.0f), INDEX_NONE)
		];
	}
	else
	{
		const USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
		for (int32 TurretIndex = 0; TurretIndex < PreparedTurrets.Num(); ++TurretIndex)
		{
			const FSCTDPreparedTurretRecord& TurretRecord = PreparedTurrets[TurretIndex];
			FString RoleLabel = TEXT("READY");
			FSCTDOwnedTurretPartRecord WeaponPart;
			if (PartsRepository && PartsRepository->FindPart(TurretRecord.WeaponPartInstanceId, WeaponPart))
			{
				RoleLabel = WeaponPart.DisplayName;
			}

			ScrollBox->AddSlot()
			.Padding(FMargin(5.0f, 0.0f))
			[
				BuildTurretCard(
					TurretRecord.DisplayName.IsEmpty() ? FString::Printf(TEXT("TURRET %d"), TurretIndex + 1) : TurretRecord.DisplayName,
					RoleLabel,
					AccentColors[TurretIndex % UE_ARRAY_COUNT(AccentColors)],
					TurretIndex)
			];
		}
	}

	return SAssignNew(BuildListBox, SBox)
		.WidthOverride(640.0f)
		.HeightOverride(96.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FrameColor)
			.Padding(FMargin(1.0f))
			[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(PanelColor)
			.Padding(FMargin(8.0f))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					ScrollBox
				]
				+ SOverlay::Slot()
				[
					SAssignNew(BuildDragCaptureBorder, SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor(FLinearColor::Transparent)
					.Padding(FMargin(0.0f))
					.OnMouseButtonDown(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleBuildListMouseButtonDown))
					.OnMouseButtonUp(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleBuildListMouseButtonUp))
					.OnMouseMove(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleBuildListMouseMove))
				]
			]
			]
		];
}

TSharedRef<SWidget> USCTDHUDWidget::BuildTurretCard(const FString& TurretName, const FString& RoleLabel, const FLinearColor& AccentColor, int32 TurretIndex)
{
	const FLinearColor CardColor(0.018f, 0.030f, 0.033f, 0.92f);
	const FLinearColor TextColor(0.78f, 1.00f, 0.94f, 1.00f);
	const FLinearColor DimTextColor(0.38f, 0.78f, 0.78f, 1.00f);

	TSharedPtr<SBox> ItemBox;
	TSharedRef<SWidget> Card =
		SAssignNew(ItemBox, SBox)
		.WidthOverride(112.0f)
		.HeightOverride(78.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda(
				[this, AccentColor, TurretIndex]()
				{
					const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
					const bool bSelected = CurrentDefenseManager && TurretIndex != INDEX_NONE && CurrentDefenseManager->GetSelectedBuildTurretIndex() == TurretIndex;
					return FSlateColor(AccentColor.CopyWithNewOpacity(bSelected ? 1.0f : 0.78f));
				})))
			.Padding(FMargin(1.0f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda(
					[this, CardColor, TurretIndex]()
					{
						const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
						const bool bSelected = CurrentDefenseManager && TurretIndex != INDEX_NONE && CurrentDefenseManager->GetSelectedBuildTurretIndex() == TurretIndex;
						return FSlateColor(bSelected ? FLinearColor(0.105f, 0.145f, 0.145f, 0.98f) : CardColor);
					})))
				.Padding(FMargin(8.0f, 7.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(TurretName))
						.ColorAndOpacity(TextColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(0.0f, 5.0f, 0.0f, 4.0f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
						.BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.24f))
						.Padding(FMargin(0.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(RoleLabel))
						.ColorAndOpacity(DimTextColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];

	BuildItemBoxes.Add(ItemBox);
	return Card;
}

FReply USCTDHUDWidget::HandleBuildListMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	bIsDraggingBuildList = true;
	LastBuildDragScreenX = MouseEvent.GetScreenSpacePosition().X;
	BuildDragStartScreenX = LastBuildDragScreenX;
	bBuildListDragExceededClickThreshold = false;
	BuildScrollVelocity = 0.0f;
	return BuildDragCaptureBorder
		? FReply::Handled().CaptureMouse(BuildDragCaptureBorder.ToSharedRef())
		: FReply::Handled();
}

FReply USCTDHUDWidget::HandleBuildListMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !bIsDraggingBuildList)
	{
		return FReply::Unhandled();
	}

	bIsDraggingBuildList = false;
	if (!bBuildListDragExceededClickThreshold && DefenseManager.IsValid() && !PreparedTurrets.IsEmpty())
	{
		const int32 ClickedIndex = GetBuildTurretIndexAtScreenPosition(MyGeometry, MouseEvent.GetScreenSpacePosition());
		if (PreparedTurrets.IsValidIndex(ClickedIndex))
		{
			DefenseManager->SetSelectedBuildTurretIndex(ClickedIndex);
		}
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FReply USCTDHUDWidget::HandleBuildListMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bIsDraggingBuildList || !BuildScrollBox)
	{
		HoveredBuildTurretIndex = GetBuildTurretIndexAtScreenPosition(MyGeometry, MouseEvent.GetScreenSpacePosition());
		if (PreparedTurrets.IsValidIndex(HoveredBuildTurretIndex))
		{
			ShowBuildTurretStatsPopup(HoveredBuildTurretIndex);
		}
		else
		{
			HideBuildTurretStatsPopup();
		}
		UpdateBuildTurretStatsPopupPosition();
		return FReply::Unhandled();
	}

	const float CurrentScreenX = MouseEvent.GetScreenSpacePosition().X;
	const float DeltaX = CurrentScreenX - LastBuildDragScreenX;
	LastBuildDragScreenX = CurrentScreenX;
	if (FMath::Abs(CurrentScreenX - BuildDragStartScreenX) > 6.0f)
	{
		bBuildListDragExceededClickThreshold = true;
	}

	BuildScrollOffset = FMath::Clamp(BuildScrollOffset - DeltaX, 0.0f, BuildScrollBox->GetScrollOffsetOfEnd());
	BuildScrollVelocity = -DeltaX / FMath::Max(FApp::GetDeltaTime(), KINDA_SMALL_NUMBER);
	BuildScrollBox->SetScrollOffset(BuildScrollOffset);
	BuildScrollOffset = BuildScrollBox->GetScrollOffset();
	HoveredBuildTurretIndex = INDEX_NONE;
	HideBuildTurretStatsPopup();
	return FReply::Handled();
}

void USCTDHUDWidget::TickBuildListInertia(float DeltaTime)
{
	if (!BuildScrollBox)
	{
		return;
	}

	if (bIsDraggingBuildList)
	{
		return;
	}

	if (FMath::Abs(BuildScrollVelocity) <= 1.0f)
	{
		BuildScrollVelocity = 0.0f;
		return;
	}

	BuildScrollOffset = FMath::Clamp(BuildScrollOffset + BuildScrollVelocity * DeltaTime, 0.0f, BuildScrollBox->GetScrollOffsetOfEnd());
	BuildScrollBox->SetScrollOffset(BuildScrollOffset);
	BuildScrollOffset = BuildScrollBox->GetScrollOffset();
	BuildScrollVelocity = FMath::FInterpTo(BuildScrollVelocity, 0.0f, DeltaTime, 7.5f);
	UpdateBuildTurretStatsPopupPosition();
}

void USCTDHUDWidget::RefreshBuildTurretStatsPopupHover()
{
	if (!BuildDragCaptureBorder || bIsDraggingBuildList)
	{
		if (bIsDraggingBuildList)
		{
			HoveredBuildTurretIndex = INDEX_NONE;
			HideBuildTurretStatsPopup();
		}
		return;
	}

	const FGeometry& BuildListGeometry = BuildDragCaptureBorder->GetCachedGeometry();
	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
	HoveredBuildTurretIndex = GetBuildTurretIndexAtScreenPosition(BuildListGeometry, CursorPosition);
	if (PreparedTurrets.IsValidIndex(HoveredBuildTurretIndex))
	{
		ShowBuildTurretStatsPopup(HoveredBuildTurretIndex);
	}
	else
	{
		HideBuildTurretStatsPopup();
	}
}

int32 USCTDHUDWidget::GetBuildTurretIndexAtScreenPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPosition) const
{
	if (!BuildScrollBox || PreparedTurrets.IsEmpty())
	{
		return INDEX_NONE;
	}

	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(ScreenPosition);
	const FVector2D LocalSize = MyGeometry.GetLocalSize();
	if (LocalPosition.X < 0.0f || LocalPosition.X > LocalSize.X || LocalPosition.Y < 0.0f || LocalPosition.Y > LocalSize.Y)
	{
		return INDEX_NONE;
	}

	const float ScrolledX = LocalPosition.X + BuildScrollBox->GetScrollOffset();
	const float SlotWidth = FMath::Max(1.0f, BuildCardWidth + BuildCardSpacing);
	const float XWithinSlot = FMath::Fmod(ScrolledX, SlotWidth);
	if (XWithinSlot < 0.0f || XWithinSlot > BuildCardWidth)
	{
		return INDEX_NONE;
	}

	const int32 TurretIndex = FMath::FloorToInt(ScrolledX / SlotWidth);
	return PreparedTurrets.IsValidIndex(TurretIndex) ? TurretIndex : INDEX_NONE;
}

void USCTDHUDWidget::ShowBuildTurretStatsPopup(int32 TurretIndex)
{
	if (!PreparedTurrets.IsValidIndex(TurretIndex) || !UserRepository)
	{
		HideBuildTurretStatsPopup();
		return;
	}

	const USCTDPartsRepository* PartsRepository = UserRepository->GetPartsRepository();
	if (!PartsRepository)
	{
		HideBuildTurretStatsPopup();
		return;
	}

	const FSCTDPreparedTurretRecord& TurretRecord = PreparedTurrets[TurretIndex];
	FSCTDOwnedTurretPartRecord BasePart;
	FSCTDOwnedTurretPartRecord WeaponPart;
	FSCTDOwnedTurretPartRecord ControlPart;
	if (!PartsRepository->FindResolvedPart(TurretRecord.BasePartInstanceId, BasePart)
		|| !PartsRepository->FindResolvedPart(TurretRecord.WeaponPartInstanceId, WeaponPart)
		|| !PartsRepository->FindResolvedPart(TurretRecord.ControlPartInstanceId, ControlPart))
	{
		HideBuildTurretStatsPopup();
		return;
	}

	if (BuildTurretStatsPopupWidget && BuildTurretStatsPopupIndex != TurretIndex)
	{
		BuildTurretStatsPopupWidget->RemoveFromParent();
		BuildTurretStatsPopupWidget = nullptr;
		BuildTurretStatsPopupIndex = INDEX_NONE;
	}

	FSCTDTurretPopupStats PopupStats;
	PopupStats.DisplayName = TurretRecord.DisplayName;
	PopupStats.MountType = BasePart.MountType;
	PopupStats.MaxHealth = BasePart.BaseHealth;
	PopupStats.Defense = BasePart.Defense;
	PopupStats.SelfRepairPerSecond = BasePart.SelfRepairPerSecond;
	PopupStats.MinAttackDamage = WeaponPart.MinAttackDamage;
	PopupStats.MaxAttackDamage = WeaponPart.MaxAttackDamage;
	PopupStats.AttackSpeed = WeaponPart.AttackSpeed;
	PopupStats.AttackRangeTiles = WeaponPart.AttackRange;
	PopupStats.AreaAttackRangeTiles = WeaponPart.AreaAttackRange;
	PopupStats.CriticalChance = WeaponPart.CriticalChance;
	PopupStats.CriticalDamageMultiplier = WeaponPart.CriticalDamageMultiplier;
	PopupStats.AttackAttribute = WeaponPart.AttackAttribute;
	PopupStats.StatusEffectChances = WeaponPart.StatusEffectChances;
	PopupStats.AIProfileId = ControlPart.AIProfileId;
	PopupStats.TargetingAI = ControlPart.TargetingAI;
	PopupStats.BasePartName = BasePart.DisplayName;
	PopupStats.WeaponPartName = WeaponPart.DisplayName;
	PopupStats.ControlPartName = ControlPart.DisplayName;

	if (!BuildTurretStatsPopupWidget)
	{
		BuildTurretStatsPopupWidget = CreateWidget<UTurretStatsPopupWidget>(GetOwningPlayer(), UTurretStatsPopupWidget::StaticClass());
		if (BuildTurretStatsPopupWidget)
		{
			BuildTurretStatsPopupWidget->SetStats(PopupStats);
			BuildTurretStatsPopupWidget->AddToViewport(125);
			BuildTurretStatsPopupIndex = TurretIndex;
		}
	}

	if (!BuildTurretStatsPopupWidget)
	{
		return;
	}

	BuildTurretStatsPopupWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateBuildTurretStatsPopupPosition();
}

void USCTDHUDWidget::HideBuildTurretStatsPopup()
{
	if (BuildTurretStatsPopupWidget)
	{
		BuildTurretStatsPopupWidget->RemoveFromParent();
		BuildTurretStatsPopupWidget = nullptr;
	}
	BuildTurretStatsPopupIndex = INDEX_NONE;
}

void USCTDHUDWidget::UpdateBuildTurretStatsPopupPosition()
{
	if (!BuildTurretStatsPopupWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	BuildTurretStatsPopupWidget->SetPositionInViewport(FVector2D(MouseX + 18.0f, MouseY - 188.0f), false);
}

TSharedRef<SWidget> USCTDHUDWidget::BuildLevelUpChoiceOverlay()
{
	const FLinearColor FrameColor(1.0f, 0.62f, 0.16f, 0.92f);
	const FLinearColor PanelColor(0.030f, 0.020f, 0.014f, 0.96f);
	const FLinearColor TextColor(1.0f, 0.88f, 0.62f, 1.0f);

	return SNew(SBox)
		.WidthOverride(720.0f)
		.HeightOverride(280.0f)
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([this]()
		{
			const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
			return CurrentDefenseManager && CurrentDefenseManager->IsLevelUpChoicePending()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FrameColor)
			.Padding(FMargin(2.0f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(PanelColor)
				.Padding(FMargin(24.0f, 20.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSCTDMarqueeText)
						.Text(FText::FromString(TEXT("LEVEL UP - SELECT ONE")))
						.ColorAndOpacity(TextColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(0.0f, 18.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 0.0f, 10.0f, 0.0f)
						[
							BuildLevelUpCard(0)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(10.0f, 0.0f)
						[
							BuildLevelUpCard(1)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(10.0f, 0.0f, 0.0f, 0.0f)
						[
							BuildLevelUpCard(2)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> USCTDHUDWidget::BuildLevelUpCard(int32 CardIndex)
{
	const FLinearColor CardColor(0.10f, 0.055f, 0.020f, 1.0f);
	const FLinearColor HoverColor(0.18f, 0.095f, 0.030f, 1.0f);
	const FLinearColor TextColor(1.0f, 0.88f, 0.62f, 1.0f);

	return SNew(SButton)
		.ButtonColorAndOpacity(CardColor)
		.OnClicked(FOnClicked::CreateUObject(this, &USCTDHUDWidget::HandleLevelUpCardClicked, CardIndex))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda(
				[CardColor, HoverColor]()
				{
					return FSlateColor(CardColor);
				})))
			.Padding(FMargin(16.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SSCTDMarqueeText)
					.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &USCTDHUDWidget::BuildLevelUpCardText, CardIndex)))
					.ColorAndOpacity(TextColor)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(SSCTDMarqueeText)
					.Text(FText::FromString(TEXT("INSTANT REWARD")))
					.ColorAndOpacity(FLinearColor(1.0f, 0.68f, 0.30f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.Justification(ETextJustify::Center)
				]
			]
		];
}

FReply USCTDHUDWidget::HandleLevelUpCardClicked(int32 CardIndex)
{
	if (DefenseManager.IsValid())
	{
		DefenseManager->SelectLevelUpCard(CardIndex);
	}

	return FReply::Handled();
}

TSharedRef<SWidget> USCTDHUDWidget::BuildDefenseResultOverlay()
{
	const FLinearColor FrameColor(0.20f, 0.56f, 1.0f, 0.95f);
	const FLinearColor PanelColor(0.010f, 0.014f, 0.020f, 0.98f);
	const FLinearColor TextColor(0.84f, 0.92f, 1.0f, 1.0f);

	return SNew(SBox)
		.WidthOverride(760.0f)
		.HeightOverride(520.0f)
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([this]()
		{
			const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
			return CurrentDefenseManager && CurrentDefenseManager->IsDefenseFinished()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FrameColor)
			.Padding(2.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(PanelColor)
				.Padding(FMargin(24.0f, 20.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSCTDMarqueeText)
						.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &USCTDHUDWidget::BuildDefenseResultTitleText)))
						.ColorAndOpacity(TextColor)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 18.0f)
					[
						SNew(SSCTDMarqueeText)
						.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &USCTDHUDWidget::BuildDefenseResultScrapText)))
						.ColorAndOpacity(FLinearColor(1.0f, 0.84f, 0.36f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 17))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(0.0f, 0.0f, 0.0f, 16.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								BuildDefenseDamageTable()
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 22.0f, 0.0f, 0.0f)
							.HAlign(HAlign_Center)
							[
								BuildDefenseRecordTable()
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SButton)
						.ContentPadding(FMargin(38.0f, 10.0f))
						.ButtonColorAndOpacity(FLinearColor(0.06f, 0.16f, 0.28f, 1.0f))
						.OnClicked(FOnClicked::CreateUObject(this, &USCTDHUDWidget::HandleDefenseResultConfirmClicked))
						[
							SNew(SSCTDMarqueeText)
							.Text(FText::FromString(TEXT("CONFIRM")))
							.ColorAndOpacity(TextColor)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> USCTDHUDWidget::BuildDefenseDamageTable()
{
	TArray<FSCTDResultTableColumn> Columns;
	Columns.Add({ TEXT("TYPE"), 220.0f, ETextJustify::Left });
	Columns.Add({ TEXT("COUNT"), 66.0f, ETextJustify::Right });
	Columns.Add({ TEXT("DMG_PER_SCRAB"), 150.0f, ETextJustify::Right });
	Columns.Add({ TEXT("DMG_RATIO"), 78.0f, ETextJustify::Right });

	auto GetRows = [this]() -> TArray<FDefenseDamageSummaryRow>
	{
		const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
		return CurrentDefenseManager
			? CurrentDefenseManager->GetSortedDamageSummaryRows()
			: TArray<FDefenseDamageSummaryRow>();
	};

	TSharedRef<SWidget> TableWidget = BuildSCTDResultTable(
		Columns,
		6,
		[GetRows](int32 RowIndex, int32 ColumnIndex)
		{
			const TArray<FDefenseDamageSummaryRow> Rows = GetRows();
			if (Rows.IsEmpty() && RowIndex == 0)
			{
				return ColumnIndex == 0 ? FString(TEXT("NO DAMAGE DATA")) : FString(TEXT("-"));
			}
			if (!Rows.IsValidIndex(RowIndex))
			{
				return FString(TEXT("-"));
			}

			const FDefenseDamageSummaryRow& Row = Rows[RowIndex];
			switch (ColumnIndex)
			{
			case 0:
				return Row.TypeName.Left(28);
			case 1:
				return FString::FromInt(Row.Count);
			case 2:
				return Row.TotalBuildCost > 0 ? FString::Printf(TEXT("%.2f"), Row.DamagePerScrab) : FString(TEXT("-"));
			case 3:
				return FString::Printf(TEXT("%.1f%%"), Row.DamageRatioPercent);
			default:
				return FString(TEXT("-"));
			}
		},
		[GetRows](int32 RowIndex)
		{
			const TArray<FDefenseDamageSummaryRow> Rows = GetRows();
			return (Rows.IsEmpty() && RowIndex == 0) || Rows.IsValidIndex(RowIndex)
				? EVisibility::Visible
				: EVisibility::Collapsed;
		},
		[](int32)
		{
			return false;
		});

	return BuildSCTDResultTableSection(TEXT("DAMAGE STATISTICS"), TableWidget);
}

TSharedRef<SWidget> USCTDHUDWidget::BuildDefenseRecordTable()
{
	TArray<FSCTDResultTableColumn> Columns;
	Columns.Add({ TEXT("RANKING"), 76.0f, ETextJustify::Left });
	Columns.Add({ TEXT("ELAPSED TIME"), 118.0f, ETextJustify::Left });
	Columns.Add({ TEXT("CONSUMED SCRAB"), 142.0f, ETextJustify::Right });
	Columns.Add({ TEXT("RECORD TIMESTAMP"), 178.0f, ETextJustify::Left });

	auto GetRows = [this]() -> TArray<FDefenseRecordSummaryRow>
	{
		const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
		return CurrentDefenseManager
			? CurrentDefenseManager->GetDefenseRecordSummaryRows()
			: TArray<FDefenseRecordSummaryRow>();
	};

	TSharedRef<SWidget> TableWidget = BuildSCTDResultTable(
				Columns,
				3,
				[GetRows](int32 RowIndex, int32 ColumnIndex)
				{
					const TArray<FDefenseRecordSummaryRow> Rows = GetRows();
					if (!Rows.IsValidIndex(RowIndex))
					{
						return ColumnIndex == 1 ? FString(TEXT("--:--")) : FString(TEXT("-"));
					}

					const FDefenseRecordSummaryRow& Row = Rows[RowIndex];
					switch (ColumnIndex)
					{
					case 0:
						return FString::FromInt(Row.Ranking);
					case 1:
						return Row.ElapsedTimeText;
					case 2:
						return FString::FromInt(Row.ConsumedScrap);
					case 3:
						return Row.Timestamp;
					default:
						return FString(TEXT("-"));
					}
				},
				[](int32)
				{
					return EVisibility::Visible;
				},
				[GetRows](int32 RowIndex)
				{
					const TArray<FDefenseRecordSummaryRow> Rows = GetRows();
					return Rows.IsValidIndex(RowIndex) && Rows[RowIndex].bIsCurrentRecord;
				});

	return BuildSCTDResultTableSection(TEXT("CLEAR RECORDS"), TableWidget);
}

FReply USCTDHUDWidget::HandleDefenseResultConfirmClicked()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/Lobby")));
	return FReply::Handled();
}

FText USCTDHUDWidget::BuildHealthText() const
{
	const UStatusComponent* StatusComponent = ObservedStatus.Get();
	const int32 CurrentHealth = StatusComponent ? FMath::RoundToInt(StatusComponent->GetCurrentHealth()) : 0;
	const int32 MaxHealth = StatusComponent ? FMath::RoundToInt(StatusComponent->GetMaxHealth()) : 0;
	return FText::FromString(FString::Printf(TEXT("HULL  %03d / %03d"), CurrentHealth, MaxHealth));
}

FText USCTDHUDWidget::BuildBoostText() const
{
	const UStatusComponent* StatusComponent = ObservedStatus.Get();
	const int32 CurrentBoost = StatusComponent ? FMath::RoundToInt(StatusComponent->GetCurrentBoost()) : 0;
	const int32 MaxBoost = StatusComponent ? FMath::RoundToInt(StatusComponent->GetMaxBoost()) : 0;
	return FText::FromString(FString::Printf(TEXT("BOOST %03d / %03d"), CurrentBoost, MaxBoost));
}

FText USCTDHUDWidget::BuildModeText() const
{
	const AFlyingPlayerPawn* FlyingPawn = Cast<AFlyingPlayerPawn>(ObservedPawn.Get());
	if (!FlyingPawn)
	{
		return FText::FromString(TEXT("TACTICAL LINK"));
	}

	switch (FlyingPawn->GetAircraftState())
	{
	case EPlayerAircraftState::Idle:
		return FText::FromString(TEXT("MODE: HOLD"));
	case EPlayerAircraftState::Flying:
		return FText::FromString(TEXT("MODE: VECTOR"));
	case EPlayerAircraftState::Attack:
		return FText::FromString(TEXT("MODE: STRIKE"));
	case EPlayerAircraftState::Building:
		return FText::FromString(TEXT("MODE: BUILD"));
	default:
		return FText::FromString(TEXT("MODE: UNKNOWN"));
	}
}

FText USCTDHUDWidget::BuildScrapText() const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	const int32 Scrap = CurrentDefenseManager ? CurrentDefenseManager->GetCurrentScrap() : 0;
	return FText::FromString(FString::Printf(TEXT("SCRAP %04d"), Scrap));
}

FText USCTDHUDWidget::BuildDefenseTimeText() const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	const float ElapsedSeconds = CurrentDefenseManager ? CurrentDefenseManager->GetDefenseElapsedSeconds() : 0.0f;
	const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(ElapsedSeconds));
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60));
}

FText USCTDHUDWidget::BuildMonsterCountText() const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	const int32 AliveCount = CurrentDefenseManager ? CurrentDefenseManager->GetCurrentAliveMonsterCount() : 0;
	const int32 SpawnedCount = CurrentDefenseManager ? CurrentDefenseManager->GetSpawnedMonsterCount() : 0;
	const int32 TotalSpawnCount = CurrentDefenseManager ? CurrentDefenseManager->GetTotalMonsterSpawnCount() : 0;
	return FText::FromString(FString::Printf(TEXT("MONSTERS %d (%d / %d)"), AliveCount, SpawnedCount, TotalSpawnCount));
}

FText USCTDHUDWidget::BuildLevelText() const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	const int32 Level = CurrentDefenseManager ? CurrentDefenseManager->GetDefensePlayerLevel() : 1;
	const int32 MaxLevel = CurrentDefenseManager ? CurrentDefenseManager->GetMaxDefensePlayerLevel() : 50;
	const int32 CurrentExp = CurrentDefenseManager ? FMath::FloorToInt(CurrentDefenseManager->GetCurrentLevelExperience()) : 0;
	const int32 RequiredExp = CurrentDefenseManager ? FMath::CeilToInt(CurrentDefenseManager->GetNextLevelExperienceRequirement()) : 1000;
	if (Level >= MaxLevel)
	{
		return FText::FromString(FString::Printf(TEXT("LV %02d / %02d  EXP MAX"), Level, MaxLevel));
	}

	return FText::FromString(FString::Printf(TEXT("LV %02d  EXP %d / %d"), Level, CurrentExp, RequiredExp));
}

FText USCTDHUDWidget::BuildLevelUpCardText(int32 CardIndex) const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	if (!CurrentDefenseManager)
	{
		return FText::FromString(TEXT("SCRAP +0"));
	}

	const TArray<int32>& Options = CurrentDefenseManager->GetLevelUpScrapCardOptions();
	const int32 ScrapAmount = Options.IsValidIndex(CardIndex) ? Options[CardIndex] : 0;
	return FText::FromString(FString::Printf(TEXT("SCRAP +%d"), ScrapAmount));
}

FText USCTDHUDWidget::BuildDefenseResultTitleText() const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	const bool bVictory = CurrentDefenseManager && CurrentDefenseManager->IsDefenseVictory();
	return FText::FromString(bVictory ? TEXT("DEFENSE SUCCESS") : TEXT("DEFENSE FAILED"));
}

FText USCTDHUDWidget::BuildDefenseResultScrapText() const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	const int32 TotalScrap = CurrentDefenseManager ? CurrentDefenseManager->GetTotalUserScrapAfterResult() : 0;
	const int32 RecoveredScrap = CurrentDefenseManager ? CurrentDefenseManager->GetRecoveredScrapThisRun() : 0;
	return FText::FromString(FString::Printf(TEXT("YOU EARNED +%d SCRAB! (TOTAL %d)"), RecoveredScrap, TotalScrap));
}

FText USCTDHUDWidget::BuildConstructionText() const
{
	const ADefenseManager* CurrentDefenseManager = DefenseManager.Get();
	if (!CurrentDefenseManager || !CurrentDefenseManager->IsConstructionActive())
	{
		return FText::FromString(TEXT("BUILD: IDLE"));
	}

	const FString StateLabel = CurrentDefenseManager->IsConstructionPaused() ? TEXT("RELEASING") : TEXT("BUILDING");
	return FText::FromString(FString::Printf(TEXT("%s: %s %.0f%%"),
		*StateLabel,
		*CurrentDefenseManager->GetConstructionLabel(),
		CurrentDefenseManager->GetConstructionProgressRatio() * 100.0f));
}
