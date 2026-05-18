#include "FloatingDamageTextLibrary.h"

#include "Blueprint/UserWidget.h"
#include "FloatingDamageTextWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

void SCTDFloatingDamageText::Spawn(
	AActor* DamagedActor,
	float DamageAmount,
	float VerticalOffset,
	const FVector2D& RelativeScreenOffset,
	const FVector2D& RandomScreenRadius,
	float TransitionY)
{
	if (!DamagedActor || DamageAmount <= 0.0f)
	{
		return;
	}

	UWorld* World = DamagedActor->GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	if (UFloatingDamageTextWidget* DamageWidget = CreateWidget<UFloatingDamageTextWidget>(PlayerController, UFloatingDamageTextWidget::StaticClass()))
	{
		DamageWidget->InitializeDamageText(
			DamagedActor->GetActorLocation() + FVector(0.0f, 0.0f, VerticalOffset),
			DamageAmount,
			RelativeScreenOffset,
			RandomScreenRadius,
			TransitionY);
		DamageWidget->AddToViewport(131);
	}
}
