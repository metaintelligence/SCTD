#include "StatusComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCTDStatus, Log, All);

UStatusComponent::UStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = FMath::Max(0.0f, MaxHealth);
	CurrentBoost = bUsesBoost ? FMath::Max(0.0f, MaxBoost) : 0.0f;
}

float UStatusComponent::GetHealthRatio() const
{
	return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;
}

void UStatusComponent::SetHealth(float NewHealth)
{
	const float ClampedHealth = FMath::Clamp(NewHealth, 0.0f, FMath::Max(0.0f, MaxHealth));
	if (FMath::IsNearlyEqual(CurrentHealth, ClampedHealth))
	{
		return;
	}

	CurrentHealth = ClampedHealth;
	UE_LOG(LogSCTDStatus, Log, TEXT("%s health changed: %.2f / %.2f"),
		*GetNameSafe(GetOwner()),
		CurrentHealth,
		MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UStatusComponent::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f)
	{
		return;
	}

	SetHealth(CurrentHealth - DamageAmount);
}

float UStatusComponent::GetBoostRatio() const
{
	return bUsesBoost && MaxBoost > 0.0f ? FMath::Clamp(CurrentBoost / MaxBoost, 0.0f, 1.0f) : 0.0f;
}

void UStatusComponent::SetBoost(float NewBoost)
{
	if (!bUsesBoost)
	{
		return;
	}

	const float ClampedBoost = FMath::Clamp(NewBoost, 0.0f, FMath::Max(0.0f, MaxBoost));
	if (FMath::IsNearlyEqual(CurrentBoost, ClampedBoost))
	{
		return;
	}

	CurrentBoost = ClampedBoost;
	UE_LOG(LogSCTDStatus, Verbose, TEXT("%s boost changed: %.2f / %.2f"),
		*GetNameSafe(GetOwner()),
		CurrentBoost,
		MaxBoost);
	OnBoostChanged.Broadcast(CurrentBoost, MaxBoost);
}

void UStatusComponent::RecoverBoost(float DeltaSeconds, float RecoveryRate)
{
	if (!bUsesBoost || DeltaSeconds <= 0.0f || RecoveryRate <= 0.0f)
	{
		return;
	}

	SetBoost(CurrentBoost + RecoveryRate * DeltaSeconds);
}

bool UStatusComponent::ConsumeBoost(float DeltaSeconds, float ConsumeRate)
{
	if (!bUsesBoost)
	{
		return false;
	}

	if (DeltaSeconds <= 0.0f || ConsumeRate <= 0.0f)
	{
		return CurrentBoost > 0.0f;
	}

	if (CurrentBoost <= 0.0f)
	{
		return false;
	}

	SetBoost(CurrentBoost - ConsumeRate * DeltaSeconds);
	return true;
}
