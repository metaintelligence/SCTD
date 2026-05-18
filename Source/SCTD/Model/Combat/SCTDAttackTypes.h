#pragma once

#include "CoreMinimal.h"
#include "SCTDAttackTypes.generated.h"

UENUM(BlueprintType)
enum class ESCTDAttackAttribute : uint8
{
	Physical UMETA(DisplayName = "Physical"),
	Fire UMETA(DisplayName = "Fire"),
	Lightning UMETA(DisplayName = "Lightning"),
	Frost UMETA(DisplayName = "Frost")
};

UENUM(BlueprintType)
enum class ESCTDStatusEffectType : uint8
{
	None UMETA(DisplayName = "None"),
	Destruction UMETA(DisplayName = "Destruction"),
	Concussion UMETA(DisplayName = "Concussion"),
	Ignite UMETA(DisplayName = "Ignite"),
	Fire UMETA(DisplayName = "Fire"),
	Stagger UMETA(DisplayName = "Stagger"),
	Execute UMETA(DisplayName = "Execute"),
	Chill UMETA(DisplayName = "Chill"),
	Freeze UMETA(DisplayName = "Freeze")
};

inline bool IsStatusEffectCompatibleWithAttribute(ESCTDAttackAttribute AttackAttribute, ESCTDStatusEffectType EffectType)
{
	switch (AttackAttribute)
	{
	case ESCTDAttackAttribute::Physical:
		return EffectType == ESCTDStatusEffectType::Destruction || EffectType == ESCTDStatusEffectType::Concussion;
	case ESCTDAttackAttribute::Fire:
		return EffectType == ESCTDStatusEffectType::Ignite || EffectType == ESCTDStatusEffectType::Fire;
	case ESCTDAttackAttribute::Lightning:
		return EffectType == ESCTDStatusEffectType::Stagger || EffectType == ESCTDStatusEffectType::Execute;
	case ESCTDAttackAttribute::Frost:
		return EffectType == ESCTDStatusEffectType::Chill || EffectType == ESCTDStatusEffectType::Freeze;
	default:
		return false;
	}
}

USTRUCT(BlueprintType)
struct FSCTDAttackDamageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDamage = 0.0f;

	float GetNormalizedMinDamage() const
	{
		return FMath::Max(0.0f, FMath::Min(MinDamage, MaxDamage));
	}

	float GetNormalizedMaxDamage() const
	{
		return FMath::Max(0.0f, FMath::Max(MinDamage, MaxDamage));
	}

	float GetAverageDamage() const
	{
		return (GetNormalizedMinDamage() + GetNormalizedMaxDamage()) * 0.5f;
	}

	float RollDamage() const
	{
		return FMath::FRandRange(GetNormalizedMinDamage(), GetNormalizedMaxDamage());
	}
};

USTRUCT(BlueprintType)
struct FSCTDStatusEffectChance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	ESCTDStatusEffectType EffectType = ESCTDStatusEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float BaseChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ChanceMultiplier = 1.0f;

	float GetFinalChance() const
	{
		return FMath::Clamp(BaseChance * FMath::Max(0.0f, ChanceMultiplier), 0.0f, 1.0f);
	}
};

USTRUCT(BlueprintType)
struct FSCTDStatusEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	ESCTDStatusEffectType EffectType = ESCTDStatusEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinDurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	float MinValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	float MaxValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FSCTDActiveStatusEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	ESCTDStatusEffectType EffectType = ESCTDStatusEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	float RemainingSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FSCTDAttackProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FSCTDAttackDamageRange DamageRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	ESCTDAttackAttribute AttackAttribute = ESCTDAttackAttribute::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Status Effect")
	TArray<FSCTDStatusEffectChance> StatusEffectChances;

	float RollDamage() const
	{
		return DamageRange.RollDamage();
	}
};
