#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LobbySceneRoot.generated.h"

UCLASS(Blueprintable)
class SCTD_API ALobbySceneRoot : public AActor
{
	GENERATED_BODY()

public:
	ALobbySceneRoot();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	bool bShowMouseCursor = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<class ULobbyWidget> LobbyWidget;

	void EnsureLobbyWidget();
};
