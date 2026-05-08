#pragma once

#include "CoreMinimal.h"
#include "SCTDTurretPartTypes.generated.h"

UENUM(BlueprintType)
enum class ESCTDTurretPartGrade : uint8
{
	Common UMETA(DisplayName = "Common"),
	Advanced UMETA(DisplayName = "Advanced"),
	Rare UMETA(DisplayName = "Rare"),
	Heroic UMETA(DisplayName = "Heroic")
};

USTRUCT(BlueprintType)
struct FSCTDTurretPartOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Option")
	FName OptionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Option")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Option")
	float Value = 0.0f;
};
