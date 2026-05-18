#include "InventorySceneRoot.h"

#include "GameFramework/PlayerController.h"
#include "InventoryWidget.h"
#include "Model/Repository/SCTDUserRepository.h"

AInventorySceneRoot::AInventorySceneRoot()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AInventorySceneRoot::BeginPlay()
{
	Super::BeginPlay();
	EnsureInventoryWidget();
}

void AInventorySceneRoot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
		InventoryWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AInventorySceneRoot::EnsureInventoryWidget()
{
	if (InventoryWidget)
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

	UserRepository = USCTDUserRepository::CreateUserRepository(this);
	InventoryWidget = CreateWidget<UInventoryWidget>(PlayerController, UInventoryWidget::StaticClass());
	if (InventoryWidget)
	{
		InventoryWidget->SetUserRepository(UserRepository);
		InventoryWidget->AddToViewport(100);
	}
}
