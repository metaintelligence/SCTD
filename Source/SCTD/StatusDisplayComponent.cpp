#include "StatusDisplayComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StatusBarWidget.h"
#include "StatusComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSCTDStatusDisplay, Log, All);

UStatusDisplayComponent::UStatusDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStatusDisplayComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	StatusComponent = Owner->FindComponentByClass<UStatusComponent>();
	if (!StatusComponent)
	{
		UE_LOG(LogSCTDStatusDisplay, Warning, TEXT("%s has no StatusComponent."), *GetNameSafe(Owner));
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
	if (!PlayerController)
	{
		UE_LOG(LogSCTDStatusDisplay, Warning, TEXT("%s cannot create status widget: no player controller."), *GetNameSafe(Owner));
		return;
	}

	StatusWidget = CreateWidget<UStatusBarWidget>(PlayerController, UStatusBarWidget::StaticClass());
	if (!StatusWidget)
	{
		UE_LOG(LogSCTDStatusDisplay, Warning, TEXT("%s failed to create status widget."), *GetNameSafe(Owner));
		return;
	}

	StatusWidget->SetStatusComponent(StatusComponent);
	StatusWidget->SetShowBoost(bShowBoost && StatusComponent->UsesBoost());
	StatusWidget->SetBarColors(
		ParseHexColor(HealthFillHexColor, FLinearColor::Red),
		ParseHexColor(BoostFillHexColor, FLinearColor::Green));
	StatusWidget->SetGaugeLayout(GaugeWidth, GaugeHeight, GaugeOffsetPx);
	StatusWidget->AddToViewport(20);
	StatusWidget->SetVisibility(ESlateVisibility::Collapsed);
	UpdateWidgetLocation();

	StatusComponent->OnHealthChanged.AddDynamic(this, &UStatusDisplayComponent::HandleHealthChanged);
	StatusComponent->OnBoostChanged.AddDynamic(this, &UStatusDisplayComponent::HandleBoostChanged);
	RefreshWidget();
}

void UStatusDisplayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StatusComponent)
	{
		StatusComponent->OnHealthChanged.RemoveDynamic(this, &UStatusDisplayComponent::HandleHealthChanged);
		StatusComponent->OnBoostChanged.RemoveDynamic(this, &UStatusDisplayComponent::HandleBoostChanged);
	}

	if (StatusWidget)
	{
		StatusWidget->RemoveFromParent();
		StatusWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UStatusDisplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!StatusWidget)
	{
		return;
	}

	UpdateWidgetLocation();

	const UWorld* World = GetWorld();
	if (World && StatusWidget->GetVisibility() != ESlateVisibility::Collapsed && World->GetTimeSeconds() >= HideAtTimeSeconds)
	{
		StatusWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UStatusDisplayComponent::HandleHealthChanged(float CurrentValue, float MaxValue)
{
	RefreshWidget();
	ShowTemporarily();
}

void UStatusDisplayComponent::HandleBoostChanged(float CurrentValue, float MaxValue)
{
	if (!bShowBoost)
	{
		return;
	}

	RefreshWidget();
	ShowTemporarily();
}

void UStatusDisplayComponent::ShowTemporarily()
{
	if (!StatusWidget)
	{
		return;
	}

	const UWorld* World = GetWorld();
	HideAtTimeSeconds = World ? World->GetTimeSeconds() + VisibleSecondsAfterChange : VisibleSecondsAfterChange;
	StatusWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateWidgetLocation();
}

void UStatusDisplayComponent::RefreshWidget()
{
	if (!StatusWidget)
	{
		return;
	}

	StatusWidget->SetStatusComponent(StatusComponent);
	StatusWidget->SetShowBoost(bShowBoost && StatusComponent && StatusComponent->UsesBoost());
	StatusWidget->SetBarColors(
		ParseHexColor(HealthFillHexColor, FLinearColor::Red),
		ParseHexColor(BoostFillHexColor, FLinearColor::Green));
	StatusWidget->SetGaugeLayout(GaugeWidth, GaugeHeight, GaugeOffsetPx);
	StatusWidget->Refresh();
}

void UStatusDisplayComponent::UpdateWidgetLocation()
{
	if (!StatusWidget)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
	if (!PlayerController)
	{
		return;
	}

	FVector Origin = Owner->GetActorLocation();
	FVector Extent = FVector::ZeroVector;
	Owner->GetActorBounds(false, Origin, Extent);
	const FVector WorldLocation = Origin + FVector(RelativeOffset.X, RelativeOffset.Y, Extent.Z + RelativeOffset.Z);

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, ScreenPosition, false);
	if (!bProjected)
	{
		StatusWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ScreenPosition -= GetWidgetSize() * 0.5f;
	StatusWidget->SetPositionInViewport(ScreenPosition, false);
}

FVector2D UStatusDisplayComponent::GetWidgetSize() const
{
	const float ClampedGaugeWidth = FMath::Max(1.0f, GaugeWidth);
	const float ClampedGaugeHeight = FMath::Max(1.0f, GaugeHeight);
	const float ClampedInnerPadding = FMath::Max(0.0f, GaugeOffsetPx);
	const bool bUsesBoostGauge = bShowBoost && StatusComponent && StatusComponent->UsesBoost();
	const float TotalHeight = bUsesBoostGauge ? (ClampedGaugeHeight * 2.0f) + ClampedInnerPadding : ClampedGaugeHeight;
	return FVector2D(ClampedGaugeWidth, TotalHeight);
}

FLinearColor UStatusDisplayComponent::ParseHexColor(const FString& HexColor, const FLinearColor& FallbackColor) const
{
	FString NormalizedHex = HexColor.TrimStartAndEnd();
	NormalizedHex.RemoveFromStart(TEXT("#"));
	NormalizedHex.RemoveFromStart(TEXT("0x"));
	NormalizedHex.RemoveFromStart(TEXT("0X"));

	if (NormalizedHex.Len() != 6 && NormalizedHex.Len() != 8)
	{
		return FallbackColor;
	}

	for (const TCHAR Character : NormalizedHex)
	{
		if (!FChar::IsHexDigit(Character))
		{
			return FallbackColor;
		}
	}

	const FColor ParsedColor = FColor::FromHex(NormalizedHex);
	return FLinearColor(ParsedColor);
}
