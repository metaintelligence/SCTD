#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Model/Repository/SCTDRepositoryTypes.h"
#include "SCTDDefenseTurret.generated.h"

class ABaseMonster;
class AHexGridManager;
class USceneComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTurretStatsPopupWidget;

UCLASS(Blueprintable)
class SCTD_API ASCTDDefenseTurret : public AActor
{
	GENERATED_BODY()

public:
	ASCTDDefenseTurret();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Defense|Turret")
	void InitializeFromRecords(const FSCTDPreparedTurretRecord& TurretRecord, const FSCTDOwnedTurretPartRecord& BasePart, const FSCTDOwnedTurretPartRecord& WeaponPart, const FSCTDOwnedTurretPartRecord& ControlPart);

	UFUNCTION(BlueprintPure, Category = "Defense|Turret")
	float GetCurrentHealth() const { return CurrentHealth; }

	const FString& GetDisplayName() const { return DisplayName; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TurretMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense|Turret", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRangeTiles = 4.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AHexGridManager> HexGridManager;

	UPROPERTY(Transient)
	TObjectPtr<UTurretStatsPopupWidget> StatsPopupWidget;

	FString DisplayName;
	FString BasePartName;
	FString WeaponPartName;
	FString ControlPartName;
	float MaxHealth = 0.0f;
	float CurrentHealth = 0.0f;
	ESCTDTurretMountType MountType = ESCTDTurretMountType::Tower;
	float Defense = 0.0f;
	float SelfRepairPerSecond = 0.0f;
	float MinAttackDamage = 0.0f;
	float MaxAttackDamage = 0.0f;
	float AttackSpeed = 0.0f;
	float AreaAttackRangeTiles = 0.0f;
	float CriticalChance = 0.0f;
	float CriticalDamageMultiplier = 1.5f;
	float PhysicalDamageBonusRatio = 0.0f;
	float FireDamageBonusRatio = 0.0f;
	float LightningDamageBonusRatio = 0.0f;
	float FrostDamageBonusRatio = 0.0f;
	ESCTDAttackAttribute AttackAttribute = ESCTDAttackAttribute::Physical;
	TArray<FSCTDStatusEffectChance> StatusEffectChances;
	TArray<FSCTDStatusEffectSpec> StatusEffectSpecs;
	FName AIProfileId = NAME_None;
	ESCTDTargetingAI TargetingAI = ESCTDTargetingAI::Closer;
	float AttackCooldownRemaining = 0.0f;
	bool bDynamicDialogOpen = false;

	ABaseMonster* FindTarget() const;
	bool IsBetterTarget(ABaseMonster* Candidate, ABaseMonster* CurrentBest, int32 CandidateTileDistance, int32 BestTileDistance) const;
	int32 GetTileDistanceToMonster(const ABaseMonster* Monster) const;
	int32 GetTileDistanceBetweenActors(const AActor* FirstActor, const AActor* SecondActor) const;
	void CacheHexGridManager();
	void ConfigureHoverCollision();
	void UpdateDynamicDialogState();
	bool IsCursorOverTurretScreenProjection(APlayerController* PlayerController) const;
	void UpdateStatsPopupLocation();
	void ShowStatsPopup();
	void HideStatsPopup();
	float RollAttackDamage() const;
	float ApplyCriticalRoll(float DamageAmount) const;
	float CalculateDefenseDamageMultiplier() const;
	void ApplySelfRepair(float DeltaSeconds);
	void RollStatusEffectsForTarget(ABaseMonster* Target) const;

	UFUNCTION()
	void HandleTurretBeginCursorOver(UPrimitiveComponent* TouchedComponent);

	UFUNCTION()
	void HandleTurretEndCursorOver(UPrimitiveComponent* TouchedComponent);
};
