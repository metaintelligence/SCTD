#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DefenseManager.generated.h"

class ABaseMonster;
class AHexGridManager;

USTRUCT(BlueprintType)
struct FMonsterSpawnSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Spawn")
	TSubclassOf<ABaseMonster> MonsterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Spawn", meta = (ClampMin = "0", UIMin = "0"))
	int32 SpawnCount = 0;
};

UCLASS(Blueprintable)
class SCTD_API ADefenseManager : public AActor
{
	GENERATED_BODY()

public:
	ADefenseManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Defense")
	void StartDefense();

	UFUNCTION(BlueprintCallable, Category = "Defense")
	void StopDefense();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Flow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DefenseDurationSeconds = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Spawn", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MonsterSpawnIntervalSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Spawn")
	TArray<FMonsterSpawnSlot> MonsterSpawnSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Grid")
	TObjectPtr<AHexGridManager> HexGridManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Spawn")
	TArray<int32> SpawnTileIndexList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Flow")
	bool bStartOnBeginPlay = true;

private:
	bool bDefenseRunning = false;
	float DefenseElapsedSeconds = 0.0f;
	float SpawnElapsedSeconds = 0.0f;

	UPROPERTY()
	TArray<FMonsterSpawnSlot> RuntimeMonsterSpawnSlots;

	void CacheHexGridManager();
	void TickSpawning(float DeltaSeconds);
	bool HasRemainingMonsterSpawns() const;
	ABaseMonster* SpawnRandomMonster();
	TSubclassOf<ABaseMonster> PickMonsterClassFromSlots();
	bool GetRandomSpawnLocation(FVector& OutSpawnLocation) const;
};
