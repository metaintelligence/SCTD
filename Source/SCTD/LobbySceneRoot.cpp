#include "LobbySceneRoot.h"

#include "GameFramework/PlayerController.h"
#include "LobbyWidget.h"

ALobbySceneRoot::ALobbySceneRoot()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALobbySceneRoot::BeginPlay()
{
	Super::BeginPlay();
	EnsureLobbyWidget();
}

void ALobbySceneRoot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LobbyWidget)
	{
		LobbyWidget->RemoveFromParent();
		LobbyWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ALobbySceneRoot::EnsureLobbyWidget()
{
	if (LobbyWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = bShowMouseCursor;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	LobbyWidget = CreateWidget<ULobbyWidget>(PlayerController, ULobbyWidget::StaticClass());
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport(100);
	}
}
