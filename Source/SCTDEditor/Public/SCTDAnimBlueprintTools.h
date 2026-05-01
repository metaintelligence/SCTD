#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SCTDAnimBlueprintTools.generated.h"

class UAnimBlueprint;
class UAnimationAsset;

UCLASS()
class SCTDEDITOR_API USCTDAnimBlueprintTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SCTD|Editor")
	static bool BuildSingleSequenceAnimGraph(UAnimBlueprint* AnimBlueprint, UAnimationAsset* AnimationAsset, FString& OutMessage);

	UFUNCTION(BlueprintCallable, Category = "SCTD|Editor")
	static bool BuildMonsterStateBlendAnimGraph(UAnimBlueprint* AnimBlueprint, UAnimationAsset* IdleAnimation, UAnimationAsset* WalkingAnimation, UAnimationAsset* AttackAnimation, UAnimationAsset* DeathAnimation, FString& OutMessage);
};
