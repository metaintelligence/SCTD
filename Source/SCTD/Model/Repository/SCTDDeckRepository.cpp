#include "SCTDDeckRepository.h"

#include "SCTDUserRepository.h"
#include "SCTDUserSaveGame.h"

void USCTDDeckRepository::Initialize(USCTDUserRepository* NewUserRepository)
{
	UserRepository = NewUserRepository;
}

FGuid USCTDDeckRepository::CreateDeck(const FString& DisplayName)
{
	USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame || !CanCreateDeck())
	{
		return FGuid();
	}

	FSCTDTurretDeckRecord NewDeck;
	NewDeck.DeckId = FGuid::NewGuid();
	NewDeck.DisplayName = DisplayName.IsEmpty()
		? FString::Printf(TEXT("Deck %d"), SaveGame->TurretDecks.Num() + 1)
		: DisplayName;
	SaveGame->TurretDecks.Add(NewDeck);
	return NewDeck.DeckId;
}

bool USCTDDeckRepository::RemoveDeck(const FGuid& DeckId)
{
	USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame || !DeckId.IsValid())
	{
		return false;
	}

	const int32 RemovedCount = SaveGame->TurretDecks.RemoveAll(
		[&DeckId](const FSCTDTurretDeckRecord& DeckRecord)
		{
			return DeckRecord.DeckId == DeckId;
		});
	return RemovedCount > 0;
}

FGuid USCTDDeckRepository::AddTurretToDeck(const FGuid& DeckId, const FSCTDPreparedTurretRecord& TurretRecord)
{
	FSCTDTurretDeckRecord* DeckRecord = FindMutableDeck(DeckId);
	if (!DeckRecord || DeckRecord->Turrets.Num() >= MaxTurretsPerDeck)
	{
		return FGuid();
	}

	FSCTDPreparedTurretRecord NewTurret = TurretRecord;
	if (!NewTurret.InstanceId.IsValid())
	{
		NewTurret.InstanceId = FGuid::NewGuid();
	}

	DeckRecord->Turrets.Add(NewTurret);
	return NewTurret.InstanceId;
}

bool USCTDDeckRepository::UpdateTurretInDeck(const FGuid& DeckId, const FSCTDPreparedTurretRecord& TurretRecord)
{
	FSCTDTurretDeckRecord* DeckRecord = FindMutableDeck(DeckId);
	if (!DeckRecord || !TurretRecord.InstanceId.IsValid())
	{
		return false;
	}

	for (FSCTDPreparedTurretRecord& ExistingTurret : DeckRecord->Turrets)
	{
		if (ExistingTurret.InstanceId == TurretRecord.InstanceId)
		{
			ExistingTurret = TurretRecord;
			return true;
		}
	}

	return false;
}

bool USCTDDeckRepository::RemoveTurretFromDeck(const FGuid& DeckId, const FGuid& TurretInstanceId)
{
	FSCTDTurretDeckRecord* DeckRecord = FindMutableDeck(DeckId);
	if (!DeckRecord || !TurretInstanceId.IsValid())
	{
		return false;
	}

	const int32 RemovedCount = DeckRecord->Turrets.RemoveAll(
		[&TurretInstanceId](const FSCTDPreparedTurretRecord& TurretRecord)
		{
			return TurretRecord.InstanceId == TurretInstanceId;
		});
	return RemovedCount > 0;
}

bool USCTDDeckRepository::MoveTurretInDeck(const FGuid& DeckId, const FGuid& TurretInstanceId, int32 Direction)
{
	FSCTDTurretDeckRecord* DeckRecord = FindMutableDeck(DeckId);
	if (!DeckRecord || !TurretInstanceId.IsValid() || Direction == 0)
	{
		return false;
	}

	const int32 CurrentIndex = DeckRecord->Turrets.IndexOfByPredicate(
		[&TurretInstanceId](const FSCTDPreparedTurretRecord& TurretRecord)
		{
			return TurretRecord.InstanceId == TurretInstanceId;
		});
	if (CurrentIndex == INDEX_NONE)
	{
		return false;
	}

	const int32 TargetIndex = FMath::Clamp(CurrentIndex + Direction, 0, DeckRecord->Turrets.Num() - 1);
	if (TargetIndex == CurrentIndex)
	{
		return false;
	}

	DeckRecord->Turrets.Swap(CurrentIndex, TargetIndex);
	return true;
}

bool USCTDDeckRepository::FindDeck(const FGuid& DeckId, FSCTDTurretDeckRecord& OutDeckRecord) const
{
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame || !DeckId.IsValid())
	{
		return false;
	}

	if (const FSCTDTurretDeckRecord* FoundDeck = SaveGame->TurretDecks.FindByPredicate(
		[&DeckId](const FSCTDTurretDeckRecord& DeckRecord)
		{
			return DeckRecord.DeckId == DeckId;
		}))
	{
		OutDeckRecord = *FoundDeck;
		return true;
	}

	return false;
}

TArray<FSCTDTurretDeckRecord> USCTDDeckRepository::GetDecks() const
{
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	return SaveGame ? SaveGame->TurretDecks : TArray<FSCTDTurretDeckRecord>();
}

bool USCTDDeckRepository::CanCreateDeck() const
{
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	return SaveGame && SaveGame->TurretDecks.Num() < MaxDeckCount;
}

bool USCTDDeckRepository::CanAddTurretToDeck(const FGuid& DeckId) const
{
	const FSCTDTurretDeckRecord* DeckRecord = FindMutableDeck(DeckId);
	return DeckRecord && DeckRecord->Turrets.Num() < MaxTurretsPerDeck;
}

FSCTDTurretDeckRecord* USCTDDeckRepository::FindMutableDeck(const FGuid& DeckId) const
{
	USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame || !DeckId.IsValid())
	{
		return nullptr;
	}

	return SaveGame->TurretDecks.FindByPredicate(
		[&DeckId](const FSCTDTurretDeckRecord& DeckRecord)
		{
			return DeckRecord.DeckId == DeckId;
		});
}
