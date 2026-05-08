#pragma once

#include "CoreMinimal.h"
#include "SCTDTurretPartTypes.h"
#include "UObject/Object.h"
#include "SCTDTurretPart.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API USCTDTurretPart : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Turret Part")
	ESCTDTurretPartGrade GetGrade() const;

	UFUNCTION(BlueprintPure, Category = "Turret Part")
	int32 GetAdditionalOptionCount() const;

	UFUNCTION(BlueprintPure, Category = "Turret Part")
	static ESCTDTurretPartGrade GetGradeFromAdditionalOptionCount(int32 OptionCount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part", meta = (ClampMin = "0", UIMin = "0"))
	int32 BuildCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BuildTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret Part")
	TArray<FSCTDTurretPartOption> AdditionalOptions;
};
