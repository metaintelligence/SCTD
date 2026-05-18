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
	float Defense = 0.0f;
	float MinAttackDamage = 0.0f;
	float MaxAttackDamage = 0.0f;
	float AttackSpeed = 0.0f;
	ESCTDAttackAttribute AttackAttribute = ESCTDAttackAttribute::Physical;
	TArray<FSCTDStatusEffectChance> StatusEffectChances;
	FName AIProfileId = NAME_None;
	float AttackCooldownRemaining = 0.0f;
	bool bDynamicDialogOpen = false;

	ABaseMonster* FindTarget() const;
	int32 GetTileDistanceToMonster(const ABaseMonster* Monster) const;
	void CacheHexGridManager();
	void ConfigureHoverCollision();
	void UpdateDynamicDialogState();
	bool IsCursorOverTurretScreenProjection(APlayerController* PlayerController) const;
	void UpdateStatsPopupLocation();
	void ShowStatsPopup();
	void HideStatsPopup();
	float RollAttackDamage() const;
	void RollStatusEffectsForTarget(ABaseMonster* Target) const;

	UFUNCTION()
	void HandleTurretBeginCursorOver(UPrimitiveComponent* TouchedComponent);

	UFUNCTION()
	void HandleTurretEndCursorOver(UPrimitiveComponent* TouchedComponent);
};
