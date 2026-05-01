#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BaseMonster.h"
#include "MonsterAnimInstance.generated.h"

class UAnimSequence;

UCLASS(Blueprintable, BlueprintType)
class SCTD_API UMonsterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Monster|Animation")
	void SetMonsterVisualState(EMonsterVisualState NewVisualState, UAnimSequence* NewAnimation, bool bNewLooping, float NewPlayRate, float NewBlendSeconds);

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Animation")
	EMonsterVisualState MonsterVisualState = EMonsterVisualState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Animation")
	TObjectPtr<UAnimSequence> CurrentAnimation;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Animation")
	bool bLooping = true;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Animation")
	float PlayRate = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Animation")
	float BlendSeconds = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Animation")
	float StateElapsedSeconds = 0.0f;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
