#include "SCTDUserRepository.h"

#include "Kismet/GameplayStatics.h"
#include "SCTDDeckRepository.h"
#include "SCTDPartsRepository.h"
#include "SCTDUserSaveGame.h"

USCTDUserRepository* USCTDUserRepository::CreateUserRepository(UObject* Outer, const FString& SlotName, int32 UserIndex)
{
	UObject* SafeOuter = Outer ? Outer : GetTransientPackage();
	USCTDUserRepository* Repository = NewObject<USCTDUserRepository>(SafeOuter);
	Repository->Initialize(SlotName, UserIndex);
	Repository->LoadOrCreate();
	return Repository;
}

void USCTDUserRepository::Initialize(const FString& SlotName, int32 UserIndex)
{
	SaveSlotName = SlotName.IsEmpty() ? TEXT("SCTD_User") : SlotName;
	SaveUserIndex = FMath::Max(0, UserIndex);
}

bool USCTDUserRepository::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		SaveGame = Cast<USCTDUserSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	}

	if (!SaveGame)
	{
		SaveGame = Cast<USCTDUserSaveGame>(UGameplayStatics::CreateSaveGameObject(USCTDUserSaveGame::StaticClass()));
	}

	EnsureChildRepositories();
	return SaveGame != nullptr;
}

bool USCTDUserRepository::Save()
{
	return SaveGame && UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
}

bool USCTDUserRepository::DeleteSave()
{
	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	SaveGame = nullptr;
	LoadOrCreate();
	return bDeleted;
}

int32 USCTDUserRepository::GetSelectedTurretDeckIndex() const
{
	return SaveGame ? FMath::Clamp(SaveGame->SelectedTurretDeckIndex, 0, 2) : 0;
}

void USCTDUserRepository::SetSelectedTurretDeckIndex(int32 DeckIndex)
{
	if (SaveGame)
	{
		SaveGame->SelectedTurretDeckIndex = FMath::Clamp(DeckIndex, 0, 2);
	}
}

void USCTDUserRepository::EnsureChildRepositories()
{
	if (!PartsRepository)
	{
		PartsRepository = NewObject<USCTDPartsRepository>(this);
	}
	PartsRepository->Initialize(this);

	if (!DeckRepository)
	{
		DeckRepository = NewObject<USCTDDeckRepository>(this);
	}
	DeckRepository->Initialize(this);
}
