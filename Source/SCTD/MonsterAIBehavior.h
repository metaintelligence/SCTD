#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MonsterAIBehavior.generated.h"

class ABaseMonster;

USTRUCT(BlueprintType)
struct FMonsterAIAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI")
	FVector MoveDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI")
	TObjectPtr<AActor> AttackTarget = nullptr;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API UMonsterAIBehavior : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Monster AI")
	FMonsterAIAction DecideAction(ABaseMonster* Monster, float DeltaSeconds);
	virtual FMonsterAIAction DecideAction_Implementation(ABaseMonster* Monster, float DeltaSeconds);
};

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API UBasicMonsterAIBehavior : public UMonsterAIBehavior
{
	GENERATED_BODY()

public:
	virtual FMonsterAIAction DecideAction_Implementation(ABaseMonster* Monster, float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Monster AI|Movement")
	void SetTargetMoveTileWorldLocation(const FVector& InTargetMoveTileWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Monster AI|Movement")
	void ClearTargetMoveTile();

	UFUNCTION(BlueprintPure, Category = "Monster AI|Movement")
	bool HasTargetMoveTile() const { return bHasTargetMoveTile; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI|Targets")
	FName PlayerTargetTag = TEXT("Player");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI|Targets")
	FName TowerTargetTag = TEXT("Tower");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI|Movement")
	bool bHasTargetMoveTile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI|Movement", meta = (EditCondition = "bHasTargetMoveTile"))
	FVector TargetMoveTileWorldLocation = FVector::ZeroVector;

private:
	UPROPERTY(Transient)
	FMonsterAIAction LastAction;

	AActor* FindAttackTargetInRange(ABaseMonster* Monster) const;
	bool IsAttackableTarget(const AActor* Candidate) const;
	bool IsPlayerTarget(const AActor* Candidate) const;
	bool IsTowerTarget(const AActor* Candidate) const;
	FVector GetMoveDirectionToTargetTile(const ABaseMonster* Monster) const;
};
