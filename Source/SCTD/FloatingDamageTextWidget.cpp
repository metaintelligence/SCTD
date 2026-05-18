#include "FloatingDamageTextWidget.h"

#include "GameFramework/PlayerController.h"
#include "SCTDMarqueeText.h"
#include "Styling/CoreStyle.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UFloatingDamageTextWidget::InitializeDamageText(
	const FVector& InWorldLocation,
	float InDamageAmount,
	const FVector2D& InRelativeScreenOffset,
	const FVector2D& InRandomScreenRadius,
	float InTransitionY)
{
	WorldLocation = InWorldLocation;
	DamageAmount = FMath::Max(0.0f, InDamageAmount);
	ElapsedSeconds = 0.0f;
	RelativeScreenOffset = InRelativeScreenOffset;
	TransitionY = FMath::Max(0.0f, InTransitionY);

	const FVector2D RandomRadius(
		FMath::Max(0.0f, InRandomScreenRadius.X),
		FMath::Max(0.0f, InRandomScreenRadius.Y));
	RandomScreenOffset = FVector2D(
		FMath::FRandRange(-RandomRadius.X, RandomRadius.X),
		FMath::FRandRange(-RandomRadius.Y, RandomRadius.Y));
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

TSharedRef<SWidget> UFloatingDamageTextWidget::RebuildWidget()
{
	return SAssignNew(DamageTextBlock, SSCTDMarqueeText)
		.Text(FText::FromString(BuildDamageText()))
		.ColorAndOpacity(FLinearColor(1.0f, 0.22f, 0.12f, 0.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
		.ShadowOffset(FVector2D(1.5f, 1.5f))
		.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
}

void UFloatingDamageTextWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ElapsedSeconds += InDeltaTime;
	const float Ratio = FMath::Clamp(ElapsedSeconds / FMath::Max(0.01f, DurationSeconds), 0.0f, 1.0f);
	const float Alpha = Ratio < 0.18f ? Ratio / 0.18f : 1.0f - FMath::Clamp((Ratio - 0.18f) / 0.82f, 0.0f, 1.0f);

	if (DamageTextBlock)
	{
		DamageTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.22f, 0.12f, Alpha)));
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		FVector2D ScreenPosition;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, ScreenPosition, false))
		{
			const FVector2D AnimatedOffset = RelativeScreenOffset + RandomScreenOffset + FVector2D(0.0f, -Ratio * TransitionY);
			SetPositionInViewport(ScreenPosition + AnimatedOffset, false);
		}
	}

	if (ElapsedSeconds >= DurationSeconds)
	{
		RemoveFromParent();
	}
}

FString UFloatingDamageTextWidget::BuildDamageText() const
{
	if (FMath::IsNearlyEqual(DamageAmount, FMath::RoundToFloat(DamageAmount), 0.05f))
	{
		return FString::Printf(TEXT("%.0f"), DamageAmount);
	}

	return FString::Printf(TEXT("%.1f"), DamageAmount);
}
