#include "PlayerModelComponent.h"

UPlayerModelComponent::UPlayerModelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerModelComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	CurrentFuel = MaxFuel;
}

void UPlayerModelComponent::RecoverFuel(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || FuelRecoveryRate <= 0.0f)
	{
		return;
	}

	CurrentFuel = FMath::Clamp(CurrentFuel + FuelRecoveryRate * DeltaSeconds, 0.0f, MaxFuel);
}

bool UPlayerModelComponent::ConsumeBoostFuel(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || BoostFuelConsumeRate <= 0.0f)
	{
		return CurrentFuel > 0.0f;
	}

	if (CurrentFuel <= 0.0f)
	{
		return false;
	}

	CurrentFuel = FMath::Max(0.0f, CurrentFuel - BoostFuelConsumeRate * DeltaSeconds);
	return true;
}
