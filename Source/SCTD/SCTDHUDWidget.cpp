#include "SCTDHUDWidget.h"

#include "FlyingPlayerPawn.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "StatusComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

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
								SAssignNew(ModeText, STextBlock)
								.Text(BuildModeText())
								.ColorAndOpacity(TextColor)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SAssignNew(TargetText, STextBlock)
								.Text(FText::FromString(TEXT("SCTD-01")))
								.ColorAndOpacity(DimTextColor)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SAssignNew(HealthText, STextBlock)
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
							SAssignNew(BoostText, STextBlock)
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
		];

	RefreshValues();
	return BuiltWidget;
}

void USCTDHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshObservedPawn();
	RefreshValues();
	RefreshBuildListLayout();
	TickBuildListInertia(InDeltaTime);
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

void USCTDHUDWidget::RefreshValues()
{
	const UStatusComponent* StatusComponent = ObservedStatus.Get();

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
	for (const TSharedPtr<SBox>& ItemBox : BuildItemBoxes)
	{
		if (ItemBox)
		{
			ItemBox->SetWidthOverride(CardWidth);
			ItemBox->SetHeightOverride(CardHeight);
		}
	}
}

TSharedRef<SWidget> USCTDHUDWidget::BuildTurretList()
{
	BuildItemBoxes.Reset();

	const FLinearColor PanelColor(0.006f, 0.012f, 0.014f, 0.76f);
	const FLinearColor FrameColor(0.0f, 0.92f, 0.84f, 0.62f);

	TSharedRef<SScrollBox> ScrollBox = SAssignNew(BuildScrollBox, SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.AllowOverscroll(EAllowOverscroll::Yes);

	ScrollBox->AddSlot()
	.Padding(FMargin(5.0f, 0.0f))
	[
		BuildTurretCard(TEXT("SENTRY"), TEXT("BALANCED"), FLinearColor(0.0f, 0.92f, 0.84f, 1.0f))
	];
	ScrollBox->AddSlot()
	.Padding(FMargin(5.0f, 0.0f))
	[
		BuildTurretCard(TEXT("EMBER"), TEXT("BURN"), FLinearColor(1.0f, 0.32f, 0.08f, 1.0f))
	];
	ScrollBox->AddSlot()
	.Padding(FMargin(5.0f, 0.0f))
	[
		BuildTurretCard(TEXT("FROST"), TEXT("SLOW"), FLinearColor(0.28f, 0.72f, 1.0f, 1.0f))
	];
	ScrollBox->AddSlot()
	.Padding(FMargin(5.0f, 0.0f))
	[
		BuildTurretCard(TEXT("RAIL"), TEXT("PIERCE"), FLinearColor(0.95f, 0.92f, 0.36f, 1.0f))
	];
	ScrollBox->AddSlot()
	.Padding(FMargin(5.0f, 0.0f))
	[
		BuildTurretCard(TEXT("TESLA"), TEXT("CHAIN"), FLinearColor(0.42f, 0.55f, 1.0f, 1.0f))
	];
	ScrollBox->AddSlot()
	.Padding(FMargin(5.0f, 0.0f))
	[
		BuildTurretCard(TEXT("MORTAR"), TEXT("SPLASH"), FLinearColor(0.66f, 1.0f, 0.30f, 1.0f))
	];
	ScrollBox->AddSlot()
	.Padding(FMargin(5.0f, 0.0f))
	[
		BuildTurretCard(TEXT("PULSE"), TEXT("BURST"), FLinearColor(1.0f, 0.30f, 0.62f, 1.0f))
	];

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

TSharedRef<SWidget> USCTDHUDWidget::BuildTurretCard(const FString& TurretName, const FString& RoleLabel, const FLinearColor& AccentColor)
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
			.BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.78f))
			.Padding(FMargin(1.0f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(CardColor)
				.Padding(FMargin(8.0f, 7.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
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
						SNew(STextBlock)
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
	return FReply::Handled().ReleaseMouseCapture();
}

FReply USCTDHUDWidget::HandleBuildListMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bIsDraggingBuildList || !BuildScrollBox)
	{
		return FReply::Unhandled();
	}

	const float CurrentScreenX = MouseEvent.GetScreenSpacePosition().X;
	const float DeltaX = CurrentScreenX - LastBuildDragScreenX;
	LastBuildDragScreenX = CurrentScreenX;

	BuildScrollOffset = FMath::Clamp(BuildScrollOffset - DeltaX, 0.0f, BuildScrollBox->GetScrollOffsetOfEnd());
	BuildScrollVelocity = -DeltaX / FMath::Max(FApp::GetDeltaTime(), KINDA_SMALL_NUMBER);
	BuildScrollBox->SetScrollOffset(BuildScrollOffset);
	BuildScrollOffset = BuildScrollBox->GetScrollOffset();
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
