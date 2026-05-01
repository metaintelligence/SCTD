#include "PlayerModelComponent.h"

UPlayerModelComponent::UPlayerModelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerModelComponent::BeginPlay()
{
	Super::BeginPlay();
}
