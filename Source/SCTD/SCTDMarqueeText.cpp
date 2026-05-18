#include "SCTDMarqueeText.h"

#include "Layout/ArrangedChildren.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"

void SSCTDMarqueeText::Construct(const FArguments& InArgs)
{
	ScrollSpeed = FMath::Max(1.0f, InArgs._ScrollSpeed);
	EdgePause = FMath::Max(0.0f, InArgs._EdgePause);
	OverflowPadding = FMath::Max(0.0f, InArgs._OverflowPadding);
	TextLengthHandling = InArgs._TextLengthHandling;
	ApplyClippingForTextLengthHandling();

	ChildSlot
	[
		SAssignNew(TextBlock, STextBlock)
		.Text(InArgs._Text)
		.ColorAndOpacity(InArgs._ColorAndOpacity)
		.Font(InArgs._Font)
		.ShadowOffset(InArgs._ShadowOffset)
		.ShadowColorAndOpacity(InArgs._ShadowColorAndOpacity)
		.Justification(InArgs._Justification)
	];
}

void SSCTDMarqueeText::SetText(const TAttribute<FText>& InText)
{
	if (TextBlock)
	{
		TextBlock->SetText(InText);
		ScrollTime = 0.0f;
		CurrentScrollOffset = 0.0f;
	}
}

void SSCTDMarqueeText::SetText(const FText& InText)
{
	SetText(TAttribute<FText>(InText));
}

void SSCTDMarqueeText::SetColorAndOpacity(const TAttribute<FSlateColor>& InColorAndOpacity)
{
	if (TextBlock)
	{
		TextBlock->SetColorAndOpacity(InColorAndOpacity);
	}
}

void SSCTDMarqueeText::SetColorAndOpacity(const FSlateColor& InColorAndOpacity)
{
	SetColorAndOpacity(TAttribute<FSlateColor>(InColorAndOpacity));
}

void SSCTDMarqueeText::SetFont(const TAttribute<FSlateFontInfo>& InFont)
{
	if (TextBlock)
	{
		TextBlock->SetFont(InFont);
		ScrollTime = 0.0f;
		CurrentScrollOffset = 0.0f;
	}
}

void SSCTDMarqueeText::SetShadowOffset(const TAttribute<FVector2D>& InShadowOffset)
{
	if (TextBlock)
	{
		TextBlock->SetShadowOffset(InShadowOffset);
	}
}

void SSCTDMarqueeText::SetShadowColorAndOpacity(const TAttribute<FLinearColor>& InShadowColorAndOpacity)
{
	if (TextBlock)
	{
		TextBlock->SetShadowColorAndOpacity(InShadowColorAndOpacity);
	}
}

void SSCTDMarqueeText::SetTextLengthHandling(ESCTDTextLengthHandling InTextLengthHandling)
{
	if (TextLengthHandling == InTextLengthHandling)
	{
		return;
	}

	TextLengthHandling = InTextLengthHandling;
	ScrollTime = 0.0f;
	LastOverflowDistance = 0.0f;
	CurrentScrollOffset = 0.0f;
	ApplyClippingForTextLengthHandling();
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void SSCTDMarqueeText::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	const TSharedRef<SWidget>& ChildWidget = ChildSlot.GetWidget();
	const EVisibility ChildVisibility = ChildWidget->GetVisibility();
	if (!ArrangedChildren.Accepts(ChildVisibility))
	{
		return;
	}

	const FMargin SlotPadding = ChildSlot.GetPadding();
	const FVector2D AllottedSize = AllottedGeometry.GetLocalSize();
	const FVector2D ChildDesiredSize = ChildWidget->GetDesiredSize();
	const bool bCanArrangeBeyondView = TextLengthHandling != ESCTDTextLengthHandling::Clip;
	const float ChildWidth = bCanArrangeBeyondView ? FMath::Max(AllottedSize.X, ChildDesiredSize.X) : AllottedSize.X;
	const float ChildHeight = FMath::Max(AllottedSize.Y, ChildDesiredSize.Y);
	const float CenteredOffsetX = (AllottedSize.X - ChildWidth) * 0.5f;
	const float ChildOffsetX = TextLengthHandling == ESCTDTextLengthHandling::AnimateOnOverflow
		? CurrentScrollOffset
		: CenteredOffsetX;

	ArrangedChildren.AddWidget(
		ChildVisibility,
		AllottedGeometry.MakeChild(
			ChildWidget,
			FVector2D(ChildOffsetX + SlotPadding.Left, SlotPadding.Top),
			FVector2D(ChildWidth, ChildHeight)));
}

void SSCTDMarqueeText::ApplyClippingForTextLengthHandling()
{
	switch (TextLengthHandling)
	{
	case ESCTDTextLengthHandling::AnimateOnOverflow:
	case ESCTDTextLengthHandling::Clip:
		SetClipping(EWidgetClipping::ClipToBounds);
		break;
	case ESCTDTextLengthHandling::RenderOverflow:
	default:
		SetClipping(EWidgetClipping::Inherit);
		break;
	}
}

void SSCTDMarqueeText::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!TextBlock)
	{
		return;
	}

	if (TextLengthHandling != ESCTDTextLengthHandling::AnimateOnOverflow)
	{
		ScrollTime = 0.0f;
		LastOverflowDistance = 0.0f;
		if (!FMath::IsNearlyZero(CurrentScrollOffset))
		{
			CurrentScrollOffset = 0.0f;
			Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
		}
		return;
	}

	const float AvailableWidth = AllottedGeometry.GetLocalSize().X;
	const float TextWidth = TextBlock->GetDesiredSize().X;
	const float OverflowDistance = TextWidth - AvailableWidth;
	if (OverflowDistance <= 1.0f)
	{
		ScrollTime = 0.0f;
		LastOverflowDistance = 0.0f;
		CurrentScrollOffset = 0.0f;
		return;
	}

	if (!FMath::IsNearlyEqual(OverflowDistance, LastOverflowDistance, 0.5f))
	{
		ScrollTime = 0.0f;
		LastOverflowDistance = OverflowDistance;
	}

	ScrollTime += InDeltaTime;
	CurrentScrollOffset = ComputeScrollOffset(OverflowDistance);
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

float SSCTDMarqueeText::ComputeScrollOffset(float OverflowDistance) const
{
	const float TravelDistance = OverflowDistance + OverflowPadding;
	const float TravelDuration = TravelDistance / ScrollSpeed;
	const float CycleDuration = EdgePause + TravelDuration + EdgePause;
	if (CycleDuration <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float CycleTime = FMath::Fmod(ScrollTime, CycleDuration);
	if (CycleTime < EdgePause)
	{
		return 0.0f;
	}

	const float ForwardTime = CycleTime - EdgePause;
	if (ForwardTime < TravelDuration)
	{
		return -FMath::Min(TravelDistance, ForwardTime * ScrollSpeed);
	}

	const float HoldTime = ForwardTime - TravelDuration;
	if (HoldTime < EdgePause)
	{
		return -TravelDistance;
	}

	return 0.0f;
}
