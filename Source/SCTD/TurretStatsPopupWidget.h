#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Model/Combat/SCTDAttackTypes.h"
#include "Model/Turret/Parts/SCTDTurretPartTypes.h"
#include "TurretStatsPopupWidget.generated.h"

USTRUCT(BlueprintType)
struct FSCTDTurretPopupStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	FString DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	ESCTDTurretMountType MountType = ESCTDTurretMountType::Tower;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float Defense = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float SelfRepairPerSecond = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float MinAttackDamage = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float MaxAttackDamage = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float AttackSpeed = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float AttackRangeTiles = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float AreaAttackRangeTiles = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float CriticalChance = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float CriticalDamageMultiplier = 1.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	FName AIProfileId = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	ESCTDTargetingAI TargetingAI = ESCTDTargetingAI::Closer;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	ESCTDAttackAttribute AttackAttribute = ESCTDAttackAttribute::Physical;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	TArray<FSCTDStatusEffectChance> StatusEffectChances;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	FString BasePartName;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	FString WeaponPartName;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	FString ControlPartName;
};

UCLASS()
class SCTD_API UTurretStatsPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStats(const FSCTDTurretPopupStats& NewStats);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FSCTDTurretPopupStats Stats;

	TSharedRef<SWidget> BuildStatRow(const FString& Label, const FString& Value) const;
	FString BuildMountTypeText() const;
	FString BuildAIText() const;
	FString BuildAttackAttributeText() const;
};
