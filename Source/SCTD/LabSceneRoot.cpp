#include "LabSceneRoot.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "LabTurretFusionWidget.h"
#include "Model/Repository/SCTDPartsRepository.h"
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
	if (!PartsRepository || PartsRepository->GetOwnedPartCount() > 0)
	{
		return;
	}

	auto AddMockPart = [PartsRepository](ESCTDTurretPartType PartType, const TCHAR* DefinitionId, const TCHAR* DisplayName, int32 Cost, float BuildTime, float Health, float Defense, float Damage, float Speed, const TCHAR* AIProfile)
	{
		FSCTDOwnedTurretPartRecord PartRecord;
		PartRecord.DefinitionId = DefinitionId;
		PartRecord.PartType = PartType;
		PartRecord.DisplayName = DisplayName;
		PartRecord.BuildCost = Cost;
		PartRecord.BuildTimeSeconds = BuildTime;
		PartRecord.BaseHealth = Health;
		PartRecord.Defense = Defense;
		PartRecord.AttackDamage = Damage;
		PartRecord.AttackSpeed = Speed;
		PartRecord.AIProfileId = AIProfile;
		PartsRepository->AddPart(PartRecord);
	};

	// Zombie baseline: HP 40, damage 2, cooldown 7s. Mock turret parts are roughly tuned around 10x threat scale.
	AddMockPart(ESCTDTurretPartType::Base, TEXT("mock_base_bulwark"), TEXT("Bulwark Frame"), 120, 12.0f, 400.0f, 20.0f, 0.0f, 0.0f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Base, TEXT("mock_base_light"), TEXT("Rapid Scaffold"), 80, 7.0f, 260.0f, 8.0f, 0.0f, 0.0f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Base, TEXT("mock_base_fortress"), TEXT("Fortress Chassis"), 180, 18.0f, 620.0f, 36.0f, 0.0f, 0.0f, TEXT(""));

	AddMockPart(ESCTDTurretPartType::Weapon, TEXT("mock_weapon_rifle"), TEXT("Auto Rifle Mount"), 110, 9.0f, 0.0f, 0.0f, 20.0f, 1.4f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Weapon, TEXT("mock_weapon_rail"), TEXT("Rail Spike Array"), 170, 14.0f, 0.0f, 0.0f, 48.0f, 0.45f, TEXT(""));
	AddMockPart(ESCTDTurretPartType::Weapon, TEXT("mock_weapon_flak"), TEXT("Flak Burst Pod"), 145, 11.0f, 0.0f, 0.0f, 28.0f, 0.9f, TEXT(""));

	AddMockPart(ESCTDTurretPartType::Control, TEXT("mock_control_focus"), TEXT("Focus Target AI"), 90, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, TEXT("FocusNearest"));
	AddMockPart(ESCTDTurretPartType::Control, TEXT("mock_control_swarm"), TEXT("Swarm Control AI"), 115, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, TEXT("MultiTarget"));
	AddMockPart(ESCTDTurretPartType::Control, TEXT("mock_control_elite"), TEXT("Elite Hunter AI"), 150, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, TEXT("StrongestTarget"));

	UserRepository->Save();
}
