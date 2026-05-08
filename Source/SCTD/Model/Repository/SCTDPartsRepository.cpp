#include "SCTDPartsRepository.h"

#include "SCTDUserRepository.h"
#include "SCTDUserSaveGame.h"

void USCTDPartsRepository::Initialize(USCTDUserRepository* NewUserRepository)
{
	UserRepository = NewUserRepository;
}

FGuid USCTDPartsRepository::AddPart(const FSCTDOwnedTurretPartRecord& PartRecord)
{
	USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return FGuid();
	}

	FSCTDOwnedTurretPartRecord NewRecord = PartRecord;
	if (!NewRecord.InstanceId.IsValid())
	{
		NewRecord.InstanceId = FGuid::NewGuid();
	}

	SaveGame->OwnedParts.Add(NewRecord);
	return NewRecord.InstanceId;
}

bool USCTDPartsRepository::RemovePart(const FGuid& PartInstanceId)
{
	USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame || !PartInstanceId.IsValid())
	{
		return false;
	}

	const int32 RemovedCount = SaveGame->OwnedParts.RemoveAll(
		[&PartInstanceId](const FSCTDOwnedTurretPartRecord& PartRecord)
		{
			return PartRecord.InstanceId == PartInstanceId;
		});
	return RemovedCount > 0;
}

bool USCTDPartsRepository::FindPart(const FGuid& PartInstanceId, FSCTDOwnedTurretPartRecord& OutPartRecord) const
{
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame || !PartInstanceId.IsValid())
	{
		return false;
	}

	if (const FSCTDOwnedTurretPartRecord* FoundPart = SaveGame->OwnedParts.FindByPredicate(
		[&PartInstanceId](const FSCTDOwnedTurretPartRecord& PartRecord)
		{
			return PartRecord.InstanceId == PartInstanceId;
		}))
	{
		OutPartRecord = *FoundPart;
		return true;
	}

	return false;
}

TArray<FSCTDOwnedTurretPartRecord> USCTDPartsRepository::GetOwnedParts() const
{
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	return SaveGame ? SaveGame->OwnedParts : TArray<FSCTDOwnedTurretPartRecord>();
}

TArray<FSCTDOwnedTurretPartRecord> USCTDPartsRepository::GetOwnedPartsByType(ESCTDTurretPartType PartType) const
{
	TArray<FSCTDOwnedTurretPartRecord> FilteredParts;
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return FilteredParts;
	}

	for (const FSCTDOwnedTurretPartRecord& PartRecord : SaveGame->OwnedParts)
	{
		if (PartRecord.PartType == PartType)
		{
			FilteredParts.Add(PartRecord);
		}
	}
	return FilteredParts;
}

int32 USCTDPartsRepository::GetOwnedPartCount() const
{
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	return SaveGame ? SaveGame->OwnedParts.Num() : 0;
}
