#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SCTDUserRepository.generated.h"

class USCTDDeckRepository;
class USCTDPartsRepository;
class USCTDUserSaveGame;

UCLASS(BlueprintType)
class SCTD_API USCTDUserRepository : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Repository|User")
	static USCTDUserRepository* CreateUserRepository(UObject* Outer, const FString& SlotName = TEXT("SCTD_User"), int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Repository|User")
	bool LoadOrCreate();

	UFUNCTION(BlueprintCallable, Category = "Repository|User")
	bool Save();

	UFUNCTION(BlueprintCallable, Category = "Repository|User")
	bool DeleteSave();

	UFUNCTION(BlueprintPure, Category = "Repository|User")
	USCTDPartsRepository* GetPartsRepository() const { return PartsRepository; }

	UFUNCTION(BlueprintPure, Category = "Repository|User")
	USCTDDeckRepository* GetDeckRepository() const { return DeckRepository; }

	USCTDUserSaveGame* GetSaveGame() const { return SaveGame; }

private:
	UPROPERTY(Transient)
	TObjectPtr<USCTDUserSaveGame> SaveGame;

	UPROPERTY(Transient)
	TObjectPtr<USCTDPartsRepository> PartsRepository;

	UPROPERTY(Transient)
	TObjectPtr<USCTDDeckRepository> DeckRepository;

	FString SaveSlotName = TEXT("SCTD_User");
	int32 SaveUserIndex = 0;

	void Initialize(const FString& SlotName, int32 UserIndex);
	void EnsureChildRepositories();
};
