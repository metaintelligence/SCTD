#pragma once

#include "CoreMinimal.h"
#include "SCTDRepositoryTypes.h"
#include "UObject/Object.h"
#include "SCTDPartDefinitionRepository.generated.h"

class UDataTable;

UCLASS(BlueprintType)
class SCTD_API USCTDPartDefinitionRepository : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Repository|Part Definitions")
	void SetDefinitionTables(UDataTable* NewBasePartTable, UDataTable* NewWeaponPartTable, UDataTable* NewControlPartTable);

	UFUNCTION(BlueprintCallable, Category = "Repository|Part Definitions")
	void SetOptionTable(UDataTable* NewOptionTable);

	UFUNCTION(BlueprintCallable, Category = "Repository|Part Definitions")
	void SetRarityTable(UDataTable* NewRarityTable);

	UFUNCTION(BlueprintPure, Category = "Repository|Part Definitions")
	bool FindPartDefinition(FName DefinitionId, FSCTDTurretPartDefinitionRow& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Part Definitions")
	bool FindPartDefinitionByType(ESCTDTurretPartType PartType, FName DefinitionId, FSCTDTurretPartDefinitionRow& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Part Definitions")
	TArray<FSCTDTurretPartDefinitionRow> GetPartDefinitionsByType(ESCTDTurretPartType PartType) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Part Options")
	bool FindOptionDefinition(FName OptionId, FSCTDTurretPartOptionDefinitionRow& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Part Options")
	TArray<FSCTDTurretPartOptionDefinitionRow> GetOptionsForPool(FName OptionPoolId, ESCTDTurretPartType PartType) const;

	UFUNCTION(BlueprintCallable, Category = "Repository|Part Options")
	TArray<FSCTDRolledTurretPartOption> RollOptions(FName OptionPoolId, ESCTDTurretPartType PartType, int32 OptionCount) const;

	UFUNCTION(BlueprintCallable, Category = "Repository|Part Options")
	TArray<FSCTDRolledTurretPartOption> RollOptionsForRarity(FName OptionPoolId, ESCTDTurretPartType PartType, ESCTDItemRarity Rarity, int32 OptionCount) const;

	UFUNCTION(BlueprintCallable, Category = "Repository|Item Drop")
	FSCTDItemRarityDefinitionRow RollRarity() const;

	UFUNCTION(BlueprintPure, Category = "Repository|Part Definitions")
	bool BuildOwnedPartFromDefinition(FName DefinitionId, const TArray<FSCTDRolledTurretPartOption>& RolledOptions, FSCTDOwnedTurretPartRecord& OutPartRecord) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Part Definitions")
	FSCTDOwnedTurretPartRecord ResolveOwnedPart(const FSCTDOwnedTurretPartRecord& OwnedPartRecord) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> BasePartTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> WeaponPartTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> ControlPartTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> OptionTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> RarityTable;

	const UDataTable* GetPartTable(ESCTDTurretPartType PartType) const;
	FSCTDItemRarityDefinitionRow GetDefaultRarityDefinition(ESCTDItemRarity Rarity) const;
	bool TryGetOptionValueRangeForRarity(const FSCTDTurretPartOptionDefinitionRow& OptionDefinition, ESCTDItemRarity Rarity, float& OutMinValue, float& OutMaxValue) const;
	void ApplyDefinitionToOwnedPart(const FSCTDTurretPartDefinitionRow& Definition, FSCTDOwnedTurretPartRecord& PartRecord) const;
	void ApplyRolledOptionsToOwnedPart(const TArray<FSCTDRolledTurretPartOption>& RolledOptions, FSCTDOwnedTurretPartRecord& PartRecord) const;
};
