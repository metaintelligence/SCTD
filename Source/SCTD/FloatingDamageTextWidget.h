#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingDamageTextWidget.generated.h"

class SSCTDMarqueeText;

UCLASS()
class SCTD_API UFloatingDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeDamageText(
		const FVector& InWorldLocation,
		float InDamageAmount,
		const FVector2D& InRelativeScreenOffset,
		const FVector2D& InRandomScreenRadius,
		float InTransitionY);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	FVector WorldLocation = FVector::ZeroVector;
	FVector2D RelativeScreenOffset = FVector2D::ZeroVector;
	FVector2D RandomScreenOffset = FVector2D::ZeroVector;
	float DamageAmount = 0.0f;
	float ElapsedSeconds = 0.0f;
	float DurationSeconds = 0.6f;
	float TransitionY = 25.5f;

	TSharedPtr<SSCTDMarqueeText> DamageTextBlock;

	FString BuildDamageText() const;
};
