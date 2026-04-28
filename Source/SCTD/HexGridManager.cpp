#include "HexGridManager.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"

AHexGridManager::AHexGridManager()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TileInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileInstances"));
	TileInstances->SetupAttachment(SceneRoot);
	TileInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

void AHexGridManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bGenerateOnConstruction)
	{
		GenerateGrid();
	}
}

void AHexGridManager::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateGrid();
	}
}

void AHexGridManager::GenerateGrid()
{
	ClearGrid();

	if (!TileInstances)
	{
		return;
	}

	const FVector CenterOffset = bCenterGridOnActor ? GetTileLocation(0, 0) : FVector::ZeroVector;

	for (int32 Q = -GridRadius; Q <= GridRadius; ++Q)
	{
		const int32 MinR = FMath::Max(-GridRadius, -Q - GridRadius);
		const int32 MaxR = FMath::Min(GridRadius, -Q + GridRadius);

		for (int32 R = MinR; R <= MaxR; ++R)
		{
			const FVector LocalLocation = GetTileLocation(Q, R) - CenterOffset;
			const FTransform InstanceTransform(TileRotation, LocalLocation, TileScale);
			TileInstances->AddInstance(InstanceTransform);
		}
	}
}

void AHexGridManager::ClearGrid()
{
	if (TileInstances)
	{
		TileInstances->ClearInstances();
	}
}

FVector AHexGridManager::GetTileLocation(int32 Q, int32 R) const
{
	constexpr float Sqrt3 = 1.7320508075688772f;

	if (Orientation == EHexGridOrientation::PointyTop)
	{
		const float X = TileSize * Sqrt3 * (static_cast<float>(Q) + static_cast<float>(R) * 0.5f);
		const float Y = TileSize * 1.5f * static_cast<float>(R);
		return FVector(X, Y, 0.0f);
	}

	const float X = TileSize * 1.5f * static_cast<float>(Q);
	const float Y = TileSize * Sqrt3 * (static_cast<float>(R) + static_cast<float>(Q) * 0.5f);
	return FVector(X, Y, 0.0f);
}
