#pragma once

#include "CoreMinimal.h"

class AActor;

namespace SCTDFloatingDamageText
{
	void Spawn(
		AActor* DamagedActor,
		float DamageAmount,
		float VerticalOffset = 116.0f,
		const FVector2D& RelativeScreenOffset = FVector2D(0.0f, -42.0f),
		const FVector2D& RandomScreenRadius = FVector2D(34.0f, 10.0f),
		float TransitionY = 25.5f);
}
