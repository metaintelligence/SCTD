#pragma once

#include "CoreMinimal.h"
#include "SCTDRepositoryTypes.h"
#include "UObject/Object.h"
#include "SCTDDeckRepository.generated.h"

class USCTDUserRepository;

UCLASS(BlueprintType)
class SCTD_API USCTDDeckRepository : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxDeckCount = 3;
	static constexpr int32 MaxTurretsPerDeck = 7;

	void Initialize(USCTDUserRepository* NewUserRepository);

	UFUNCTION(BlueprintCallable, Category = "Repository|Deck")
	FGuid CreateDeck(const FString& DisplayName);

	UFUNCTION(BlueprintCallable, Category = "Repository|Deck")
	bool RemoveDeck(const FGuid& DeckId);

	UFUNCTION(BlueprintCallable, Category = "Repository|Deck")
	FGuid AddTurretToDeck(const FGuid& DeckId, const FSCTDPreparedTurretRecord& TurretRecord);

	UFUNCTION(BlueprintCallable, Category = "Repository|Deck")
	bool UpdateTurretInDeck(const FGuid& DeckId, const FSCTDPreparedTurretRecord& TurretRecord);

	UFUNCTION(BlueprintCallable, Category = "Repository|Deck")
	bool RemoveTurretFromDeck(const FGuid& DeckId, const FGuid& TurretInstanceId);

	UFUNCTION(BlueprintCallable, Category = "Repository|Deck")
	bool MoveTurretInDeck(const FGuid& DeckId, const FGuid& TurretInstanceId, int32 Direction);

	UFUNCTION(BlueprintPure, Category = "Repository|Deck")
	bool FindDeck(const FGuid& DeckId, FSCTDTurretDeckRecord& OutDeckRecord) const;

	UFUNCTION(BlueprintPure, Category = "Repository|Deck")
	TArray<FSCTDTurretDeckRecord> GetDecks() const;

	UFUNCTION(BlueprintPure, Category = "Repository|Deck")
	bool CanCreateDeck() const;

	UFUNCTION(BlueprintPure, Category = "Repository|Deck")
	bool CanAddTurretToDeck(const FGuid& DeckId) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<USCTDUserRepository> UserRepository;

	FSCTDTurretDeckRecord* FindMutableDeck(const FGuid& DeckId) const;
};
