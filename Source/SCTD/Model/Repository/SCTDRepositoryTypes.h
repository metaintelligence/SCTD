#pragma once

#include "CoreMinimal.h"
#include "../Turret/Parts/SCTDTurretPartTypes.h"
#include "SCTDRepositoryTypes.generated.h"

UENUM(BlueprintType)
enum class ESCTDTurretPartType : uint8
{
	Base UMETA(DisplayName = "Base"),
	Weapon UMETA(DisplayName = "Weapon"),
	Control UMETA(DisplayName = "Control")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float BaseHealth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Base")
	float Defense = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AttackSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Weapon")
	float AttackRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Part|Control")
	FName AIProfileId = NAME_None;
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
