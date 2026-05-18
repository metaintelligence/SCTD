#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

UCLASS()
class SCTD_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedRef<SWidget> BuildModeCard(const FString& Title, const FString& Subtitle, const FLinearColor& AccentColor, const FOnClicked& OnClicked);
	TSharedRef<SWidget> BuildPlayerStatusPanel();

	FReply HandleDefenseClicked();
	FReply HandleInventoryClicked();
	FReply HandleLabClicked();
};
