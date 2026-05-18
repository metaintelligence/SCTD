#pragma once

#include "CoreMinimal.h"
#include "SCTDRepositoryTypes.h"
#include "UObject/Object.h"
#include "SCTDPartsRepository.generated.h"

class USCTDUserRepository;

UCLASS(BlueprintType)
class SCTD_API USCTDPartsRepository : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(USCTDUserRepository* NewUserRepository);

	UFUNCTION(BlueprintCallable, Category = "Repository|Parts")
	FGuid AddPart(const FSCTDOwnedTurretPartRecord& PartRecord);

	UFUNCTION(BlueprintCallable, Category = "Repository|Parts")
	FGuid AddPartByDefinitionId(FName DefinitionId, int32 RandomOptionCount = 0, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Repository|Parts")
	bool RemovePart(const FGuid& PartInstanceId);

	UFUNCTION(BlueprintPure, Category = "Repository|Parts")
	bool FindPart(const FGuid& PartInstanceId, FSCTDOwnedTurretPartRecord& OutPartRecord) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Parts")
	bool FindResolvedPart(const FGuid& PartInstanceId, FSCTDOwnedTurretPartRecord& OutPartRecord) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Parts")
	TArray<FSCTDOwnedTurretPartRecord> GetOwnedParts() const;

	UFUNCTION(BlueprintPure, Category = "Repository|Parts")
	TArray<FSCTDOwnedTurretPartRecord> GetResolvedOwnedParts() const;

	UFUNCTION(BlueprintPure, Category = "Repository|Parts")
	TArray<FSCTDOwnedTurretPartRecord> GetOwnedPartsByType(ESCTDTurretPartType PartType) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Parts")
	int32 GetOwnedPartCount() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<USCTDUserRepository> UserRepository;
};
