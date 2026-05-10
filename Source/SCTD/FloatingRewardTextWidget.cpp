#include "FloatingRewardTextWidget.h"

#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"

void UFloatingRewardTextWidget::InitializeRewardText(const FVector& InWorldLocation, int32 InScrapAmount)
{
	WorldLocation = InWorldLocation;
	ScrapAmount = InScrapAmount;
	ElapsedSeconds = 0.0f;
}

TSharedRef<SWidget> UFloatingRewardTextWidget::RebuildWidget()
{
	return SAssignNew(RewardTextBlock, STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("+%d"), ScrapAmount)))
		.ColorAndOpacity(FLinearColor(1.0f, 0.82f, 0.28f, 0.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
		.ShadowOffset(FVector2D(1.5f, 1.5f))
		.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
}

void UFloatingRewardTextWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ElapsedSeconds += InDeltaTime;
	const float Ratio = FMath::Clamp(ElapsedSeconds / FMath::Max(0.01f, DurationSeconds), 0.0f, 1.0f);
	const float Alpha = Ratio < 0.18f ? Ratio / 0.18f : 1.0f - FMath::Clamp((Ratio - 0.18f) / 0.82f, 0.0f, 1.0f);

	if (RewardTextBlock)
	{
		RewardTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.28f, Alpha)));
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		FVector2D ScreenPosition;
		const FVector AnimatedLocation = WorldLocation + FVector(0.0f, 0.0f, 90.0f + Ratio * 85.0f);
		if (PlayerController->ProjectWorldLocationToScreen(AnimatedLocation, ScreenPosition, false))
		{
			SetPositionInViewport(ScreenPosition - FVector2D(24.0f, 18.0f), false);
		}
	}

	if (ElapsedSeconds >= DurationSeconds)
	{
		RemoveFromParent();
	}
}
