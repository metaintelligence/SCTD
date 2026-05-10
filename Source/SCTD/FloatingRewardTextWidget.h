#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingRewardTextWidget.generated.h"

class STextBlock;

UCLASS()
class SCTD_API UFloatingRewardTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeRewardText(const FVector& InWorldLocation, int32 InScrapAmount);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	FVector WorldLocation = FVector::ZeroVector;
	int32 ScrapAmount = 0;
	float ElapsedSeconds = 0.0f;
	float DurationSeconds = 1.0f;

	TSharedPtr<STextBlock> RewardTextBlock;
};
