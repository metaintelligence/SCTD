#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexGridManager.h"
#include "Model/Repository/SCTDRepositoryTypes.h"
#include "DefenseManager.generated.h"

class ABaseMonster;
class AFlyingPlayerPawn;
class AHexGridManager;
class ASCTDDefenseTurret;
class USCTDUserRepository;

USTRUCT(BlueprintType)
struct FDefenseDamageSummaryRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	FString TypeName;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	float DamageSum = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	int32 TotalBuildCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	float DamageRatioPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	float DamagePerScrab = 0.0f;
};

USTRUCT(BlueprintType)
struct FDefenseRecordSummaryRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	int32 Ranking = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	FString ElapsedTimeText;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	int32 ConsumedScrap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	FString Timestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Defense|Result")
	bool bIsCurrentRecord = false;
};

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

	UFUNCTION(BlueprintPure, Category = "Defense|Resource")
	int32 GetCurrentScrap() const { return CurrentScrap; }

	UFUNCTION(BlueprintCallable, Category = "Defense|Resource")
	void AddScrap(int32 ScrapAmount);

	UFUNCTION(BlueprintCallable, Category = "Defense|Resource")
	void RefundScrap(int32 ScrapAmount);

	UFUNCTION(BlueprintCallable, Category = "Defense|Level")
	void AddExperience(int32 ExpAmount);

	UFUNCTION(BlueprintCallable, Category = "Defense|Result")
	void RegisterDamageDealt(AActor* DamageCauser, float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "Defense|Flow")
	float GetDefenseElapsedSeconds() const { return DefenseElapsedSeconds; }

	UFUNCTION(BlueprintPure, Category = "Defense|Flow")
	float GetDefenseDurationSeconds() const { return DefenseDurationSeconds; }

	UFUNCTION(BlueprintPure, Category = "Defense|Spawn")
	int32 GetCurrentAliveMonsterCount() const;

	UFUNCTION(BlueprintPure, Category = "Defense|Spawn")
	int32 GetSpawnedMonsterCount() const { return SpawnedMonsterCount; }

	UFUNCTION(BlueprintPure, Category = "Defense|Spawn")
	int32 GetTotalMonsterSpawnCount() const { return TotalMonsterSpawnCount; }

	UFUNCTION(BlueprintPure, Category = "Defense|Level")
	int32 GetDefensePlayerLevel() const { return DefensePlayerLevel; }

	UFUNCTION(BlueprintPure, Category = "Defense|Level")
	int32 GetMaxDefensePlayerLevel() const { return MaxDefensePlayerLevel; }

	UFUNCTION(BlueprintPure, Category = "Defense|Level")
	float GetCurrentLevelExperience() const { return CurrentLevelExperience; }

	UFUNCTION(BlueprintPure, Category = "Defense|Level")
	float GetNextLevelExperienceRequirement() const { return NextLevelExperienceRequirement; }

	UFUNCTION(BlueprintPure, Category = "Defense|Level")
	bool IsLevelUpChoicePending() const { return bLevelUpChoicePending; }

	const TArray<int32>& GetLevelUpScrapCardOptions() const { return LevelUpScrapCardOptions; }

	UFUNCTION(BlueprintCallable, Category = "Defense|Level")
	void SelectLevelUpCard(int32 CardIndex);

	UFUNCTION(BlueprintPure, Category = "Defense|Result")
	bool IsDefenseFinished() const { return bDefenseFinished; }

	UFUNCTION(BlueprintPure, Category = "Defense|Result")
	bool IsDefenseVictory() const { return bDefenseVictory; }

	UFUNCTION(BlueprintPure, Category = "Defense|Result")
	int32 GetCollectedScrapThisRun() const { return CollectedScrapThisRun; }

	UFUNCTION(BlueprintPure, Category = "Defense|Result")
	int32 GetRecoveredScrapThisRun() const { return RecoveredScrapThisRun; }

	UFUNCTION(BlueprintPure, Category = "Defense|Result")
	int32 GetConsumedScrapThisRun() const { return ConsumedScrapThisRun; }

	UFUNCTION(BlueprintPure, Category = "Defense|Result")
	int32 GetUserScrapBeforeResult() const { return UserScrapBeforeResult; }

	UFUNCTION(BlueprintPure, Category = "Defense|Result")
	int32 GetTotalUserScrapAfterResult() const { return TotalUserScrapAfterResult; }

	TArray<FDefenseDamageSummaryRow> GetSortedDamageSummaryRows() const;
	TArray<FDefenseRecordSummaryRow> GetDefenseRecordSummaryRows() const;

	UFUNCTION(BlueprintPure, Category = "Defense|Construction")
	bool IsConstructionActive() const { return bConstructionActive; }

	UFUNCTION(BlueprintPure, Category = "Defense|Construction")
	bool IsConstructionPaused() const { return bConstructionPaused; }

	UFUNCTION(BlueprintPure, Category = "Defense|Construction")
	float GetConstructionProgressRatio() const;

	UFUNCTION(BlueprintPure, Category = "Defense|Construction")
	FString GetConstructionLabel() const { return ConstructionTurretName; }

	UFUNCTION(BlueprintPure, Category = "Defense|Construction")
	bool GetConstructionWorldLocation(FVector& OutWorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Defense|Construction")
	int32 GetSelectedBuildTurretIndex() const { return SelectedBuildTurretIndex; }

	UFUNCTION(BlueprintCallable, Category = "Defense|Construction")
	void SetSelectedBuildTurretIndex(int32 TurretIndex);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Flow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DefenseDurationSeconds = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Resource", meta = (ClampMin = "0", UIMin = "0"))
	int32 InitialScrap = 300;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Resource", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ScrapPerSecond = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Construction")
	TSubclassOf<ASCTDDefenseTurret> DefenseTurretClass;

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
	bool bDefenseFinished = false;
	bool bDefenseVictory = false;
	float DefenseElapsedSeconds = 0.0f;
	float SpawnElapsedSeconds = 0.0f;
	float ScrapRemainder = 0.0f;
	int32 CurrentScrap = 0;
	int32 CollectedScrapThisRun = 0;
	int32 RecoveredScrapThisRun = 0;
	int32 ConsumedScrapThisRun = 0;
	int32 UserScrapBeforeResult = 0;
	int32 TotalUserScrapAfterResult = 0;
	int32 SpawnedMonsterCount = 0;
	int32 TotalMonsterSpawnCount = 0;
	int32 DefensePlayerLevel = 1;
	int32 MaxDefensePlayerLevel = 50;
	float CurrentLevelExperience = 0.0f;
	float NextLevelExperienceRequirement = 1000.0f;
	bool bLevelUpChoicePending = false;
	bool bWasSpaceDown = false;
	int32 SelectedBuildTurretIndex = 0;

	bool bConstructionActive = false;
	bool bConstructionPaused = false;
	int32 ConstructionTileIndex = INDEX_NONE;
	int32 ConstructionPaidCost = 0;
	float ConstructionElapsedSeconds = 0.0f;
	float ConstructionRequiredSeconds = 0.0f;
	FString ConstructionTurretName;
	FSCTDPreparedTurretRecord ConstructionTurretRecord;
	FSCTDOwnedTurretPartRecord ConstructionBasePart;
	FSCTDOwnedTurretPartRecord ConstructionWeaponPart;
	FSCTDOwnedTurretPartRecord ConstructionControlPart;

	UPROPERTY()
	TArray<FMonsterSpawnSlot> RuntimeMonsterSpawnSlots;

	UPROPERTY(Transient)
	TObjectPtr<USCTDUserRepository> UserRepository;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ASCTDDefenseTurret>> BuiltTurretsByTileIndex;

	TArray<int32> LevelUpScrapCardOptions;
	TMap<FString, FDefenseDamageSummaryRow> DamageSummaryByType;
	FGuid CurrentDefenseRecordId;

	void CacheHexGridManager();
	void EnsureUserRepository();
	void TickResources(float DeltaSeconds);
	void TickSpawning(float DeltaSeconds);
	void TickDefenseResult();
	void TickConstruction(float DeltaSeconds);
	void HandleBuildInput();
	bool HasRemainingMonsterSpawns() const;
	int32 GetAliveMonsterCount() const;
	ABaseMonster* SpawnRandomMonster();
	TSubclassOf<ABaseMonster> PickMonsterClassFromSlots();
	bool GetRandomSpawnLocation(FVector& OutSpawnLocation) const;
	AFlyingPlayerPawn* GetPlayerPawn() const;
	bool GetPlayerBuildTile(FHexTileSlot& OutSlot) const;
	bool TryGetSelectedDeckTurret(FSCTDPreparedTurretRecord& OutTurretRecord, FSCTDOwnedTurretPartRecord& OutBasePart, FSCTDOwnedTurretPartRecord& OutWeaponPart, FSCTDOwnedTurretPartRecord& OutControlPart) const;
	void StartConstruction(const FHexTileSlot& BuildSlot);
	void CompleteConstruction();
	void CancelConstruction(bool bRefundCost);
	void ClearConstructionState();
	void TryProcessLevelUp();
	void GenerateLevelUpCardOptions();
	void RegisterBuiltTurretType(const FString& TypeName, int32 BuildCost);
	FString GetDamageSourceTypeName(AActor* DamageCauser) const;
	void UpdateDefenseClearRecords();
	static FString FormatElapsedTime(float ElapsedSeconds);
	void FinishDefense(bool bVictory);
};
