#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "../Combat/SCTDAttackTypes.h"
#include "../Turret/Parts/SCTDTurretPartTypes.h"
#include "SCTDRepositoryTypes.generated.h"

UENUM(BlueprintType)
enum class ESCTDTurretPartType : uint8
{
	Base UMETA(DisplayName = "Base"),
	Weapon UMETA(DisplayName = "Weapon"),
	Control UMETA(DisplayName = "Control")
};

UENUM(BlueprintType)
enum class ESCTDItemRarity : uint8
{
	Common UMETA(DisplayName = "Common"),
	Advanced UMETA(DisplayName = "Advanced"),
	Rare UMETA(DisplayName = "Rare"),
	Legendary UMETA(DisplayName = "Legendary"),
	NoDrop UMETA(DisplayName = "NoDrop")
};

UENUM(BlueprintType)
enum class ESCTDPartOptionValueMode : uint8
{
	AddFlat UMETA(DisplayName = "Add Flat"),
	AddPercentOfBase UMETA(DisplayName = "Add Percent Of Base"),
	AddMultiplier UMETA(DisplayName = "Add Multiplier")
};

USTRUCT(BlueprintType)
struct FSCTDPartOptionRarityRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	ESCTDItemRarity Rarity = ESCTDItemRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	float MinValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	float MaxValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FSCTDItemRarityDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Item|Rarity")
	ESCTDItemRarity Rarity = ESCTDItemRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Item|Rarity", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Item|Rarity")
	FLinearColor DisplayColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Item|Rarity", meta = (ClampMin = "0"))
	int32 MinOptionCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Item|Rarity", meta = (ClampMin = "0"))
	int32 MaxOptionCount = 1;
};

USTRUCT(BlueprintType)
struct FSCTDRolledTurretPartOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	FName OptionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FSCTDTurretPartDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	FName DefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	ESCTDTurretPartType PartType = ESCTDTurretPartType::Base;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	int32 BuildCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	float BuildTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	FName OptionPoolId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Definition")
	ESCTDTurretMountType MountType = ESCTDTurretMountType::Tower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float BaseHealth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float Defense = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float SelfRepairPerSecond = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float MinAttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float MaxAttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AttackSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AttackRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AreaAttackRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon", meta = (ClampMin = "1.0"))
	float CriticalDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	ESCTDAttackAttribute AttackAttribute = ESCTDAttackAttribute::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	TArray<FSCTDStatusEffectChance> StatusEffectChances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Status Effect")
	TArray<FSCTDStatusEffectSpec> StatusEffectSpecs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Control")
	ESCTDTargetingAI TargetingAI = ESCTDTargetingAI::Closer;
};

USTRUCT(BlueprintType)
struct FSCTDTurretPartOptionDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	FName OptionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	FName OptionPoolId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	ESCTDTurretPartType AllowedPartType = ESCTDTurretPartType::Base;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	FName TargetStat = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	ESCTDPartOptionValueMode ValueMode = ESCTDPartOptionValueMode::AddFlat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	TArray<FSCTDPartOptionRarityRange> RarityRanges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	ESCTDStatusEffectType TargetStatusEffectType = ESCTDStatusEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	FSCTDStatusEffectSpec StatusEffectSpec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	float MinValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	float MaxValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct FSCTDOwnedTurretPartRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	FName DefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	ESCTDTurretPartType PartType = ESCTDTurretPartType::Base;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	int32 BuildCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	float BuildTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	TArray<FSCTDTurretPartOption> AdditionalOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Option")
	TArray<FSCTDRolledTurretPartOption> RolledOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	int32 UpgradeLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	ESCTDItemRarity Rarity = ESCTDItemRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	FLinearColor RarityColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part")
	ESCTDTurretMountType MountType = ESCTDTurretMountType::Tower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float BaseHealth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float Defense = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float SelfRepairPerSecond = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float MinAttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float MaxAttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AttackSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AttackRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AreaAttackRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon", meta = (ClampMin = "1.0"))
	float CriticalDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	ESCTDAttackAttribute AttackAttribute = ESCTDAttackAttribute::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	TArray<FSCTDStatusEffectChance> StatusEffectChances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Status Effect")
	TArray<FSCTDStatusEffectSpec> StatusEffectSpecs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float PhysicalDamageBonusRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float FireDamageBonusRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float LightningDamageBonusRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float FrostDamageBonusRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Control")
	FName AIProfileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Control")
	ESCTDTargetingAI TargetingAI = ESCTDTargetingAI::Closer;
};

USTRUCT(BlueprintType)
struct FSCTDPreparedTurretRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Turret")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Turret")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Turret")
	FGuid BasePartInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Turret")
	FGuid WeaponPartInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Turret")
	FGuid ControlPartInstanceId;
};

USTRUCT(BlueprintType)
struct FSCTDTurretDeckRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Deck")
	FGuid DeckId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Deck")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Deck")
	TArray<FSCTDPreparedTurretRecord> Turrets;
};
