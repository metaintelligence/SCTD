#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SCTDRepositoryTypes.h"
#include "SCTDUserSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FSCTDDefenseClearRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Defense")
	FGuid RecordId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Defense")
	float ElapsedSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Defense")
	int32 ConsumedScrap = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Defense")
	FString Timestamp;
};

UCLASS()
class SCTD_API USCTDUserSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repository|User")
	int32 SaveVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Decks")
	int32 SelectedTurretDeckIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Currency")
	int32 Scrap = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Parts")
	TArray<FSCTDOwnedTurretPartRecord> OwnedParts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Decks")
	TArray<FSCTDTurretDeckRecord> TurretDecks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repository|Defense")
	TArray<FSCTDDefenseClearRecord> DefenseClearRecords;
};
