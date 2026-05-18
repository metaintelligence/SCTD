#include "LabSceneRoot.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "LabTurretFusionWidget.h"
#include "Model/Repository/SCTDPartsRepository.h"
#include "Model/Repository/SCTDUserSaveGame.h"
#include "Model/Repository/SCTDUserRepository.h"

ALabSceneRoot::ALabSceneRoot()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALabSceneRoot::BeginPlay()
{
	Super::BeginPlay();
	EnsureLabWidget();
}

void ALabSceneRoot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LabWidget)
	{
		LabWidget->RemoveFromParent();
		LabWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ALabSceneRoot::EnsureLabWidget()
{
	if (LabWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = bShowMouseCursor;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	UserRepository = USCTDUserRepository::CreateUserRepository(this);
	SeedMockPartsIfNeeded();

	LabWidget = CreateWidget<ULabTurretFusionWidget>(PlayerController, ULabTurretFusionWidget::StaticClass());
	if (LabWidget)
	{
		LabWidget->SetUserRepository(UserRepository);
		LabWidget->AddToViewport(100);
	}
}

void ALabSceneRoot::SeedMockPartsIfNeeded()
{
	USCTDPartsRepository* PartsRepository = UserRepository ? UserRepository->GetPartsRepository() : nullptr;
	if (!PartsRepository)
	{
		return;
	}

	auto NormalizeMockPart = [](FSCTDOwnedTurretPartRecord& PartRecord)
	{
		if (PartRecord.DefinitionId == TEXT("mock_base_bulwark"))
		{
			PartRecord.BuildTimeSeconds = 6.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_base_light"))
		{
			PartRecord.BuildTimeSeconds = 3.5f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_base_fortress"))
		{
			PartRecord.BuildTimeSeconds = 9.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_weapon_rifle"))
		{
			PartRecord.BuildTimeSeconds = 4.5f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_weapon_rail"))
		{
			PartRecord.BuildTimeSeconds = 7.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_weapon_flak"))
		{
			PartRecord.BuildTimeSeconds = 5.5f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_control_focus"))
		{
			PartRecord.BuildTimeSeconds = 3.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_control_swarm"))
		{
			PartRecord.BuildTimeSeconds = 4.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_control_elite"))
		{
			PartRecord.BuildTimeSeconds = 5.0f;
		}

		if (PartRecord.DefinitionId == TEXT("mock_weapon_rifle"))
		{
			PartRecord.AttackRange = 4.0f;
			PartRecord.MinAttackDamage = 16.0f;
			PartRecord.MaxAttackDamage = 24.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_weapon_rail"))
		{
			PartRecord.AttackRange = 2.0f;
			PartRecord.MinAttackDamage = 42.0f;
			PartRecord.MaxAttackDamage = 54.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_weapon_flak"))
		{
			PartRecord.AttackRange = 6.0f;
			PartRecord.MinAttackDamage = 22.0f;
			PartRecord.MaxAttackDamage = 34.0f;
		}
		else if (PartRecord.DefinitionId == TEXT("mock_control_focus"))
		{
			PartRecord.DisplayName = TEXT("Nearest AI");
			PartRecord.AIProfileId = TEXT("Nearest");
		}
		else if (PartRecord.DefinitionId == TEXT("mock_control_swarm"))
		{
			PartRecord.DisplayName = TEXT("MaxHealth Target AI");
			PartRecord.AIProfileId = TEXT("MaxHealth");
		}
		else if (PartRecord.DefinitionId == TEXT("mock_control_elite"))
		{
			PartRecord.DisplayName = TEXT("MinHealth Target AI");
			PartRecord.AIProfileId = TEXT("MinHealth");
		}
	};

	if (PartsRepository->GetOwnedPartCount() > 0)
	{
		if (USCTDUserSaveGame* SaveGame = UserRepository ? UserRepository->GetSaveGame() : nullptr)
		{
			for (FSCTDOwnedTurretPartRecord& PartRecord : SaveGame->OwnedParts)
			{
				NormalizeMockPart(PartRecord);
			}
			UserRepository->Save();
		}
		return;
	}

	auto AddMockPart = [PartsRepository, NormalizeMockPart](ESCTDTurretPartType PartType, const TCHAR* DefinitionId, const TCHAR* DisplayName, int32 Cost, float BuildTime, float Health, float Defense, float Damage, float Speed, const TCHAR* AIProfile)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = PartType;
		PartRecord.DisplayName = DisplayName;
		PartRecord.BuildCost = Cost;
		PartRecord.BuildTimeSeconds = BuildTime;
		PartRecord.BaseHealth = Health;
		PartRecord.Defense = Defense;
		PartRecord.MinAttackDamage = Damage;
		PartRecord.MaxAttackDamage = Damage;
		PartRecord.AttackSpeed = Speed;
		PartRecord.AIProfileId = AIProfile;
		NormalizeMockPart(PartRecord);
		PartsRepository->AddPart(PartRecord);
	};

	// Zombie baseline: HP 40, damage 2, cooldown 7s. Mock turret parts are roughly tuned around 10x threat scale.
	AddMockPart(ESCTDTurretPartType::Base, TEXT("mock_base_bulwark"), TEXT("Bulwark Frame"), 120, 12.0f, 400.0f, 20.0f, 0.0f, 0.0f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Base, TEXT("mock_base_light"), TEXT("Rapid Scaffold"), 80, 7.0f, 260.0f, 8.0f, 0.0f, 0.0f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Base, TEXT("mock_base_fortress"), TEXT("Fortress Chassis"), 180, 18.0f, 620.0f, 36.0f, 0.0f, 0.0f, TEXT(""));

	AddMockPart(ESCTDTurretPartType::Weapon, TEXT("mock_weapon_rifle"), TEXT("Auto Rifle Mount"), 110, 9.0f, 0.0f, 0.0f, 20.0f, 1.4f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Weapon, TEXT("mock_weapon_rail"), TEXT("Rail Spike Array"), 170, 14.0f, 0.0f, 0.0f, 48.0f, 0.45f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Weapon, TEXT("mock_weapon_flak"), TEXT("Flak Burst Pod"), 145, 11.0f, 0.0f, 0.0f, 28.0f, 0.9f, TEXT(""));

	AddMockPart(ESCTDTurretPartType::Control, TEXT("mock_control_focus"), TEXT("Nearest AI"), 90, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, TEXT("Nearest"));
	AddMockPart(ESCTDTurretPartType::Control, TEXT("mock_control_swarm"), TEXT("MaxHealth Target AI"), 115, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, TEXT("MaxHealth"));
	AddMockPart(ESCTDTurretPartType::Control, TEXT("mock_control_elite"), TEXT("MinHealth Target AI"), 150, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, TEXT("MinHealth"));

	UserRepository->Save();
}
