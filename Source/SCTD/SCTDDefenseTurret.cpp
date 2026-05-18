#include "SCTDDefenseTurret.h"

#include "BaseMonster.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "FloatingDamageTextLibrary.h"
#include "GameFramework/PlayerController.h"
#include "HexGridManager.h"
#include "InputCoreTypes.h"
#include "TurretStatsPopupWidget.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCTDDefenseTurret, Log, All);

namespace
{
constexpr float TurretPopupHoverRadiusPixels = 76.0f;
}

ASCTDDefenseTurret::ASCTDDefenseTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	Tags.AddUnique(TEXT("Tower"));

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetupAttachment(SceneRoot);
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TurretMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	TurretMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TurretMesh->SetRelativeScale3D(FVector(10.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TurretMeshFinder(TEXT("/Game/Fab/Gun_Turret/Turret_Orange/StaticMeshes/Turret_Orange.Turret_Orange"));
	if (TurretMeshFinder.Succeeded())
	{
		TurretMesh->SetStaticMesh(TurretMeshFinder.Object);
	}
}

void ASCTDDefenseTurret::BeginPlay()
{
	Super::BeginPlay();
	CacheHexGridManager();
	ConfigureHoverCollision();
}

void ASCTDDefenseTurret::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideStatsPopup();
	Super::EndPlay(EndPlayReason);
}

void ASCTDDefenseTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentHealth <= 0.0f)
	{
		return;
	}

	AttackCooldownRemaining = FMath::Max(0.0f, AttackCooldownRemaining - DeltaSeconds);
	UpdateDynamicDialogState();
	UpdateStatsPopupLocation();
	if (AttackCooldownRemaining > 0.0f || FMath::Max(MinAttackDamage, MaxAttackDamage) <= 0.0f || AttackSpeed <= 0.0f)
	{
		return;
	}

	ABaseMonster* Target = FindTarget();
	if (!Target)
	{
		return;
	}

	FDamageEvent DamageEvent;
	const float RolledDamage = RollAttackDamage();
	Target->TakeDamage(RolledDamage, DamageEvent, nullptr, this);
	RollStatusEffectsForTarget(Target);
	AttackCooldownRemaining = 1.0f / FMath::Max(0.01f, AttackSpeed);
	UE_LOG(LogSCTDDefenseTurret, VeryVerbose, TEXT("%s attacked %s damage=%.1f range=%.1f attribute=%d ai=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Target),
		RolledDamage,
		AttackRangeTiles,
		static_cast<int32>(AttackAttribute),
		*AIProfileId.ToString());
}

void ASCTDDefenseTurret::InitializeFromRecords(const FSCTDPreparedTurretRecord& TurretRecord, const FSCTDOwnedTurretPartRecord& BasePart, const FSCTDOwnedTurretPartRecord& WeaponPart, const FSCTDOwnedTurretPartRecord& ControlPart)
{
	DisplayName = TurretRecord.DisplayName;
	BasePartName = BasePart.DisplayName;
	WeaponPartName = WeaponPart.DisplayName;
	ControlPartName = ControlPart.DisplayName;
	MaxHealth = BasePart.BaseHealth;
	Defense = BasePart.Defense;
	MinAttackDamage = WeaponPart.MinAttackDamage;
	MaxAttackDamage = WeaponPart.MaxAttackDamage;
	AttackSpeed = WeaponPart.AttackSpeed;
	AttackRangeTiles = WeaponPart.AttackRange;
	AttackAttribute = WeaponPart.AttackAttribute;
	StatusEffectChances = WeaponPart.StatusEffectChances;
	AIProfileId = ControlPart.AIProfileId;
	CurrentHealth = MaxHealth;
}

float ASCTDDefenseTurret::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHealth <= 0.0f || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float DefenseMultiplier = 100.0f / (100.0f + FMath::Max(0.0f, Defense));
	const float AppliedDamage = DamageAmount * DefenseMultiplier;
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - AppliedDamage);
	SCTDFloatingDamageText::Spawn(this, AppliedDamage, 154.0f);
	if (CurrentHealth <= 0.0f)
	{
		HideStatsPopup();
		Destroy();
	}

	return AppliedDamage;
}

ABaseMonster* ASCTDDefenseTurret::FindTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABaseMonster* BestTarget = nullptr;
	int32 BestTileDistance = TNumericLimits<int32>::Max();
	float BestHealth = 0.0f;
	const int32 MaxTileRange = FMath::Max(0, FMath::FloorToInt(AttackRangeTiles));
	const bool bMaxHealthAI = AIProfileId == TEXT("MaxHealth");
	const bool bMinHealthAI = AIProfileId == TEXT("MinHealth");

	for (TActorIterator<ABaseMonster> It(World); It; ++It)
	{
		ABaseMonster* Monster = *It;
		if (!Monster || Monster->IsActorBeingDestroyed() || Monster->GetCurrentHealth() <= 0.0f)
		{
			continue;
		}

		const int32 TileDistance = GetTileDistanceToMonster(Monster);
		if (TileDistance < 0 || TileDistance > MaxTileRange)
		{
			continue;
		}

		const float MonsterHealth = Monster->GetCurrentHealth();
		if (!BestTarget
			|| (bMaxHealthAI && MonsterHealth > BestHealth)
			|| (bMinHealthAI && MonsterHealth < BestHealth)
			|| (!bMaxHealthAI && !bMinHealthAI && TileDistance < BestTileDistance))
		{
			BestTarget = Monster;
			BestTileDistance = TileDistance;
			BestHealth = MonsterHealth;
		}
	}

	return BestTarget;
}

int32 ASCTDDefenseTurret::GetTileDistanceToMonster(const ABaseMonster* Monster) const
{
	if (!Monster || !HexGridManager)
	{
		return -1;
	}

	FHexTileSlot TurretSlot;
	FHexTileSlot MonsterSlot;
	if (!HexGridManager->FindTileSlotAtWorldLocation(GetActorLocation(), TurretSlot)
		|| !HexGridManager->FindTileSlotAtWorldLocation(Monster->GetActorLocation(), MonsterSlot))
	{
		return -1;
	}

	const int32 DeltaQ = TurretSlot.Q - MonsterSlot.Q;
	const int32 DeltaR = TurretSlot.R - MonsterSlot.R;
	const int32 DeltaS = -DeltaQ - DeltaR;
	return FMath::Max3(FMath::Abs(DeltaQ), FMath::Abs(DeltaR), FMath::Abs(DeltaS));
}

void ASCTDDefenseTurret::CacheHexGridManager()
{
	if (HexGridManager)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AHexGridManager> It(World); It; ++It)
	{
		HexGridManager = *It;
		return;
	}
}

void ASCTDDefenseTurret::ConfigureHoverCollision()
{
	if (!TurretMesh)
	{
		return;
	}

	TurretMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TurretMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	TurretMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TurretMesh->OnBeginCursorOver.AddDynamic(this, &ASCTDDefenseTurret::HandleTurretBeginCursorOver);
	TurretMesh->OnEndCursorOver.AddDynamic(this, &ASCTDDefenseTurret::HandleTurretEndCursorOver);

	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PlayerController->bShowMouseCursor = true;
		PlayerController->bEnableMouseOverEvents = true;
		PlayerController->bEnableClickEvents = true;
	}
}

void ASCTDDefenseTurret::UpdateDynamicDialogState()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController || !TurretMesh)
	{
		if (bDynamicDialogOpen)
		{
			bDynamicDialogOpen = false;
			HideStatsPopup();
		}
		return;
	}

	if (!PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		return;
	}

	FHitResult HitResult;
	const bool bHasHit = PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	const bool bHitsThisTurret = bHasHit && (HitResult.GetActor() == this || HitResult.GetComponent() == TurretMesh);
	const bool bScreenHover = IsCursorOverTurretScreenProjection(PlayerController);
	const bool bClickedThisTurret = bHitsThisTurret || bScreenHover;
	if (bClickedThisTurret)
	{
		bDynamicDialogOpen = true;
		ShowStatsPopup();
		return;
	}

	if (bDynamicDialogOpen)
	{
		bDynamicDialogOpen = false;
		HideStatsPopup();
	}
}

bool ASCTDDefenseTurret::IsCursorOverTurretScreenProjection(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return false;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	FVector Origin = GetActorLocation();
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);
	const FVector HoverWorldLocation = Origin + FVector(0.0f, 0.0f, FMath::Max(Extent.Z, 60.0f));

	FVector2D ScreenPosition;
	if (!PlayerController->ProjectWorldLocationToScreen(HoverWorldLocation, ScreenPosition, false))
	{
		return false;
	}

	const FVector2D MousePosition(MouseX, MouseY);
	return FVector2D::Distance(MousePosition, ScreenPosition) <= TurretPopupHoverRadiusPixels;
}

void ASCTDDefenseTurret::UpdateStatsPopupLocation()
{
	if (!StatsPopupWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	FVector Origin = GetActorLocation();
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);
	const FVector PopupWorldLocation = Origin + FVector(0.0f, 0.0f, Extent.Z + 220.0f);

	FVector2D ScreenPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, PopupWorldLocation, ScreenPosition, false))
	{
		StatsPopupWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	StatsPopupWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	StatsPopupWidget->SetPositionInViewport(ScreenPosition + FVector2D(18.0f, -24.0f), false);
}

void ASCTDDefenseTurret::ShowStatsPopup()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	if (!StatsPopupWidget)
	{
		FSCTDTurretPopupStats PopupStats;
		PopupStats.DisplayName = DisplayName;
		PopupStats.MaxHealth = MaxHealth;
		PopupStats.Defense = Defense;
		PopupStats.MinAttackDamage = MinAttackDamage;
		PopupStats.MaxAttackDamage = MaxAttackDamage;
		PopupStats.AttackSpeed = AttackSpeed;
		PopupStats.AttackRangeTiles = AttackRangeTiles;
		PopupStats.AttackAttribute = AttackAttribute;
		PopupStats.StatusEffectChances = StatusEffectChances;
		PopupStats.AIProfileId = AIProfileId;
		PopupStats.BasePartName = BasePartName;
		PopupStats.WeaponPartName = WeaponPartName;
		PopupStats.ControlPartName = ControlPartName;

		StatsPopupWidget = CreateWidget<UTurretStatsPopupWidget>(PlayerController, UTurretStatsPopupWidget::StaticClass());
		if (StatsPopupWidget)
		{
			StatsPopupWidget->SetStats(PopupStats);
			StatsPopupWidget->AddToViewport(120);
		}
	}

	UpdateStatsPopupLocation();
}

void ASCTDDefenseTurret::HideStatsPopup()
{
	if (StatsPopupWidget)
	{
		StatsPopupWidget->RemoveFromParent();
		StatsPopupWidget = nullptr;
	}
}

float ASCTDDefenseTurret::RollAttackDamage() const
{
	const float SafeMinDamage = FMath::Max(0.0f, FMath::Min(MinAttackDamage, MaxAttackDamage));
	const float SafeMaxDamage = FMath::Max(0.0f, FMath::Max(MinAttackDamage, MaxAttackDamage));
	if (SafeMaxDamage <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::FRandRange(SafeMinDamage, SafeMaxDamage);
}

void ASCTDDefenseTurret::RollStatusEffectsForTarget(ABaseMonster* Target) const
{
	if (!Target)
	{
		return;
	}

	for (const FSCTDStatusEffectChance& StatusEffectChance : StatusEffectChances)
	{
		if (StatusEffectChance.EffectType == ESCTDStatusEffectType::None)
		{
			continue;
		}

		const float FinalChance = StatusEffectChance.GetFinalChance();
		if (FinalChance > 0.0f && FMath::FRand() <= FinalChance)
		{
			UE_LOG(LogSCTDDefenseTurret, VeryVerbose, TEXT("%s rolled status effect %d on %s chance=%.3f"),
				*GetNameSafe(this),
				static_cast<int32>(StatusEffectChance.EffectType),
				*GetNameSafe(Target),
				FinalChance);
		}
	}
}

void ASCTDDefenseTurret::HandleTurretBeginCursorOver(UPrimitiveComponent* TouchedComponent)
{
}

void ASCTDDefenseTurret::HandleTurretEndCursorOver(UPrimitiveComponent* TouchedComponent)
{
}
