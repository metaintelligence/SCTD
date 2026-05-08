#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SCTDRepositoryTypes.h"
#include "SCTDUserSaveGame.generated.h"

UCLASS()
class SCTD_API USCTDUserSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repository|User")
	int32 SaveVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Parts")
	TArray<FSCTDOwnedTurretPartRecord> OwnedParts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Decks")
	TArray<FSCTDTurretDeckRecord> TurretDecks;
};
