#pragma once

#include "CoreMinimal.h"
#include "SCTDTurretPart.h"
#include "SCTDTurretBasePart.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API USCTDTurretBasePart : public USCTDTurretPart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Base", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Base", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Defense = 0.0f;
};
