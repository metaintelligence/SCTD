#include "MonsterAnimInstance.h"

#include "Animation/AnimSequence.h"

void UMonsterAnimInstance::SetMonsterVisualState(EMonsterVisualState NewVisualState, UAnimSequence* NewAnimation, bool bNewLooping, float NewPlayRate, float NewBlendSeconds)
{
	const bool bStateChanged = MonsterVisualState != NewVisualState || CurrentAnimation != NewAnimation;
	MonsterVisualState = NewVisualState;
	CurrentAnimation = NewAnimation;
	bLooping = bNewLooping;
	PlayRate = FMath::Max(0.01f, NewPlayRate);
	BlendSeconds = FMath::Max(0.0f, NewBlendSeconds);

	if (bStateChanged)
	{
		StateElapsedSeconds = 0.0f;
	}
}

void UMonsterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	StateElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
}
