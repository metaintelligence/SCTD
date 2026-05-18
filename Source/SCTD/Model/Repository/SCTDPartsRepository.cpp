#include "SCTDPartsRepository.h"

#include "SCTDPartDefinitionRepository.h"
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

FGuid USCTDPartsRepository::AddPartByDefinitionId(FName DefinitionId, int32 RandomOptionCount, bool bSaveImmediately)
{
	return AddPartByDefinitionIdAndRarity(DefinitionId, ESCTDItemRarity::Common, RandomOptionCount, FLinearColor::White, bSaveImmediately);
}

FGuid USCTDPartsRepository::AddPartByDefinitionIdAndRarity(FName DefinitionId, ESCTDItemRarity Rarity, int32 RandomOptionCount, const FLinearColor& RarityColor, bool bSaveImmediately)
{
	USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	USCTDPartDefinitionRepository* DefinitionRepository = UserRepository ? UserRepository->GetPartDefinitionRepository() : nullptr;
	if (!SaveGame || !DefinitionRepository || DefinitionId.IsNone())
	{
		return FGuid();
	}

	FSCTDTurretPartDefinitionRow Definition;
	if (!DefinitionRepository->FindPartDefinition(DefinitionId, Definition))
	{
		return FGuid();
	}

	const TArray<FSCTDRolledTurretPartOption> RolledOptions = DefinitionRepository->RollOptionsForRarity(
		Definition.OptionPoolId,
		Definition.PartType,
		Rarity,
		RandomOptionCount);

	FSCTDOwnedTurretPartRecord NewRecord;
	if (!DefinitionRepository->BuildOwnedPartFromDefinition(DefinitionId, RolledOptions, NewRecord))
	{
		return FGuid();
	}

	NewRecord.Rarity = Rarity;
	NewRecord.RarityColor = RarityColor;
	SaveGame->OwnedParts.Add(NewRecord);
	if (bSaveImmediately && UserRepository)
	{
		UserRepository->Save();
	}
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

bool USCTDPartsRepository::FindResolvedPart(const FGuid& PartInstanceId, FSCTDOwnedTurretPartRecord& OutPartRecord) const
{
	FSCTDOwnedTurretPartRecord FoundRecord;
	if (!FindPart(PartInstanceId, FoundRecord))
	{
		return false;
	}

	const USCTDPartDefinitionRepository* DefinitionRepository = UserRepository ? UserRepository->GetPartDefinitionRepository() : nullptr;
	OutPartRecord = DefinitionRepository ? DefinitionRepository->ResolveOwnedPart(FoundRecord) : FoundRecord;
	return true;
}

TArray<FSCTDOwnedTurretPartRecord> USCTDPartsRepository::GetOwnedParts() const
{
	const USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr;
	return SaveGame ? SaveGame->OwnedParts : TArray<FSCTDOwnedTurretPartRecord>();
}

TArray<FSCTDOwnedTurretPartRecord> USCTDPartsRepository::GetResolvedOwnedParts() const
{
	const TArray<FSCTDOwnedTurretPartRecord> OwnedParts = GetOwnedParts();
	const USCTDPartDefinitionRepository* DefinitionRepository = UserRepository ? UserRepository->GetPartDefinitionRepository() : nullptr;
	if (!DefinitionRepository)
	{
		return OwnedParts;
	}

	TArray<FSCTDOwnedTurretPartRecord> ResolvedParts;
	ResolvedParts.Reserve(OwnedParts.Num());
	for (const FSCTDOwnedTurretPartRecord& OwnedPart : OwnedParts)
	{
		ResolvedParts.Add(DefinitionRepository->ResolveOwnedPart(OwnedPart));
	}
	return ResolvedParts;
}

TArray<FSCTDOwnedTurretPartRecord> USCTDPartsRepository::GetOwnedPartsByType(ESCTDTurretPartType PartType) const
{
	TArray<FSCTDOwnedTurretPartRecord> FilteredParts;
	for (const FSCTDOwnedTurretPartRecord& PartRecord : GetResolvedOwnedParts())
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
