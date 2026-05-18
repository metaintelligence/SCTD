#pragma once

#include "CoreMinimal.h"
#include "SCTDTurretPart.h"
#include "SCTDTurretControlPart.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API USCTDTurretControlPart : public USCTDTurretPart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Control")
	ESCTDTargetingAI TargetingAI = ESCTDTargetingAI::Closer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part|Control")
	FText AIProfileDescription;
};
