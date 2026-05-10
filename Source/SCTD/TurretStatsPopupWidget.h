#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	float Defense = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float AttackDamage = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float AttackSpeed = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	float AttackRangeTiles = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret|Popup")
	FName AIProfileId = NAME_None;

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
	FString BuildAIText() const;
};
