#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;

enum class ESCTDTextLengthHandling : uint8
{
	AnimateOnOverflow,
	RenderOverflow,
	Clip
};

class SSCTDMarqueeText : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSCTDMarqueeText)
		: _ColorAndOpacity(FSlateColor::UseForeground())
		, _Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
		, _Justification(ETextJustify::Center)
		, _ScrollSpeed(34.0f)
		, _EdgePause(0.75f)
		, _OverflowPadding(18.0f)
		, _TextLengthHandling(ESCTDTextLengthHandling::RenderOverflow)
	{
	}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)
		SLATE_ATTRIBUTE(FVector2D, ShadowOffset)
		SLATE_ATTRIBUTE(FLinearColor, ShadowColorAndOpacity)
		SLATE_ARGUMENT(ETextJustify::Type, Justification)
		SLATE_ARGUMENT(float, ScrollSpeed)
		SLATE_ARGUMENT(float, EdgePause)
		SLATE_ARGUMENT(float, OverflowPadding)
		SLATE_ARGUMENT(ESCTDTextLengthHandling, TextLengthHandling)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetText(const TAttribute<FText>& InText);
	void SetText(const FText& InText);
	void SetColorAndOpacity(const TAttribute<FSlateColor>& InColorAndOpacity);
	void SetColorAndOpacity(const FSlateColor& InColorAndOpacity);
	void SetFont(const TAttribute<FSlateFontInfo>& InFont);
	void SetShadowOffset(const TAttribute<FVector2D>& InShadowOffset);
	void SetShadowColorAndOpacity(const TAttribute<FLinearColor>& InShadowColorAndOpacity);
	void SetTextLengthHandling(ESCTDTextLengthHandling InTextLengthHandling);

protected:
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedPtr<STextBlock> TextBlock;
	float ScrollSpeed = 34.0f;
	float EdgePause = 0.75f;
	float OverflowPadding = 18.0f;
	float ScrollTime = 0.0f;
	float LastOverflowDistance = 0.0f;
	float CurrentScrollOffset = 0.0f;
	ESCTDTextLengthHandling TextLengthHandling = ESCTDTextLengthHandling::RenderOverflow;

	void ApplyClippingForTextLengthHandling();
	float ComputeScrollOffset(float OverflowDistance) const;
};
