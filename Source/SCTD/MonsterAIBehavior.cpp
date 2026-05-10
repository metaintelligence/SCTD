#include "MonsterAIBehavior.h"

#include "BaseMonster.h"
#include "EngineUtils.h"
#include "FlyingPlayerPawn.h"
#include "SCTDDefenseTurret.h"

FMonsterAIAction UMonsterAIBehavior::DecideAction_Implementation(ABaseMonster* Monster, float DeltaSeconds)
{
	return FMonsterAIAction();
}

FMonsterAIAction UBasicMonsterAIBehavior::DecideAction_Implementation(ABaseMonster* Monster, float DeltaSeconds)
{
	if (!Monster)
	{
		LastAction = FMonsterAIAction();
		return LastAction;
	}

	if (AActor* AttackTarget = FindAttackTargetInRange(Monster))
	{
		if (Monster->IsAttackCooldownReady())
		{
			LastAction = FMonsterAIAction();
			LastAction.AttackTarget = AttackTarget;
			return LastAction;
		}
	}

	LastAction = FMonsterAIAction();
	LastAction.MoveDirection = GetMoveDirectionToTargetTile(Monster);
	return LastAction;
}

void UBasicMonsterAIBehavior::SetTargetMoveTileWorldLocation(const FVector& InTargetMoveTileWorldLocation)
{
	TargetMoveTileWorldLocation = InTargetMoveTileWorldLocation;
	bHasTargetMoveTile = true;
}

void UBasicMonsterAIBehavior::ClearTargetMoveTile()
{
	TargetMoveTileWorldLocation = FVector::ZeroVector;
	bHasTargetMoveTile = false;
}

AActor* UBasicMonsterAIBehavior::FindAttackTargetInRange(ABaseMonster* Monster) const
{
	UWorld* World = Monster ? Monster->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const FVector MonsterLocation = Monster->GetActorLocation();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsPlayerTarget(Candidate) || !Monster->IsTargetInAttackRange(Candidate))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(MonsterLocation, Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}

	if (BestTarget)
	{
		return BestTarget;
	}

	BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsTowerTarget(Candidate) || !Monster->IsTargetInAttackRange(Candidate))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(MonsterLocation, Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool UBasicMonsterAIBehavior::IsAttackableTarget(const AActor* Candidate) const
{
	if (!Candidate || Candidate->IsActorBeingDestroyed())
	{
		return false;
	}

	return IsPlayerTarget(Candidate) || IsTowerTarget(Candidate);
}

bool UBasicMonsterAIBehavior::IsPlayerTarget(const AActor* Candidate) const
{
	if (!Candidate || Candidate->IsActorBeingDestroyed())
	{
		return false;
	}

	return Candidate->IsA<AFlyingPlayerPawn>() || Candidate->ActorHasTag(PlayerTargetTag);
}

bool UBasicMonsterAIBehavior::IsTowerTarget(const AActor* Candidate) const
{
	if (!Candidate || Candidate->IsActorBeingDestroyed())
	{
		return false;
	}

	const ASCTDDefenseTurret* Turret = Cast<ASCTDDefenseTurret>(Candidate);
	if (Turret && Turret->GetCurrentHealth() > 0.0f)
	{
		return true;
	}

	return Candidate->ActorHasTag(TowerTargetTag);
}

FVector UBasicMonsterAIBehavior::GetMoveDirectionToTargetTile(const ABaseMonster* Monster) const
{
	if (!Monster || !bHasTargetMoveTile)
	{
		return FVector::ZeroVector;
	}

	FVector MoveDirection = TargetMoveTileWorldLocation - Monster->GetActorLocation();
	MoveDirection.Z = 0.0f;
	return MoveDirection.GetSafeNormal();
}
