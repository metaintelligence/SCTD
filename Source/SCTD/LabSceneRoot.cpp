#include "LabSceneRoot.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "LabTurretFusionWidget.h"

ALabSceneRoot::ALabSceneRoot()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALabSceneRoot::BeginPlay()
{
	Super::BeginPlay();
	EnsureLabWidget();
}

void ALabSceneRoot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LabWidget)
	{
		LabWidget->RemoveFromParent();
		LabWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ALabSceneRoot::EnsureLabWidget()
{
	if (LabWidget)
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

	LabWidget = CreateWidget<ULabTurretFusionWidget>(PlayerController, ULabTurretFusionWidget::StaticClass());
	if (LabWidget)
	{
		LabWidget->AddToViewport(100);
	}
}
