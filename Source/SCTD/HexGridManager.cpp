#include "HexGridManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

AHexGridManager::AHexGridManager()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
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
	RebuildTileSlots();

	for (const FHexTileSlot& Slot : TileSlots)
	{
		if (!Slot.Tile || !Slot.Tile->Mesh)
		{
			continue;
		}

		const FName ComponentName(*FString::Printf(TEXT("HexTileSlot_%03d"), Slot.SlotIndex));
		UStaticMeshComponent* TileComponent = NewObject<UStaticMeshComponent>(this, ComponentName, RF_Transactional);
		if (!TileComponent)
		{
			continue;
		}

		TileComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		TileComponent->SetupAttachment(SceneRoot);
		TileComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		TileComponent->SetStaticMesh(Slot.Tile->Mesh);
		TileComponent->SetRelativeLocation(Slot.LocalLocation);
		TileComponent->SetRelativeRotation(TileRotation);
		TileComponent->SetRelativeScale3D(TileScale);
		TileComponent->RegisterComponent();
		AddInstanceComponent(TileComponent);
		GeneratedTileComponents.Add(TileComponent);
	}
}

void AHexGridManager::ClearGrid()
{
	for (UStaticMeshComponent* TileComponent : GeneratedTileComponents)
	{
		if (TileComponent)
		{
			TileComponent->DestroyComponent();
		}
	}

	GeneratedTileComponents.Reset();
}

void AHexGridManager::RebuildTileSlots()
{
	const TArray<FHexTileSlot> ExistingSlots = TileSlots;
	TileSlots.Reset(GetExpectedTileSlotCount());

	TArray<FIntPoint> AxialCoordinates;
	AxialCoordinates.Reserve(GetExpectedTileSlotCount());
	AxialCoordinates.Add(FIntPoint::ZeroValue);

	const int32 AxialRadius = FMath::Max(0, GridRadius - 1);
	for (int32 Ring = 1; Ring <= AxialRadius; ++Ring)
	{
		AddRingSlots(AxialCoordinates, Ring);
	}

	const FVector CenterOffset = bCenterGridOnActor ? GetTileLocation(0, 0) : FVector::ZeroVector;
	for (int32 Index = 0; Index < AxialCoordinates.Num(); ++Index)
	{
		const int32 SlotIndex = Index + 1;
		FHexTileSlot NewSlot;
		NewSlot.SlotIndex = SlotIndex;
		NewSlot.Q = AxialCoordinates[Index].X;
		NewSlot.R = AxialCoordinates[Index].Y;
		NewSlot.Ring = FMath::Max3(FMath::Abs(NewSlot.Q), FMath::Abs(NewSlot.R), FMath::Abs(-NewSlot.Q - NewSlot.R));
		NewSlot.RingIndex = NewSlot.Ring == 0 ? 0 : Index - (1 + 3 * (NewSlot.Ring - 1) * NewSlot.Ring) + 1;
		NewSlot.LocalLocation = GetTileLocation(NewSlot.Q, NewSlot.R) - CenterOffset;

		const FHexTileSlot* ExistingSlot = ExistingSlots.FindByPredicate([SlotIndex](const FHexTileSlot& Candidate)
		{
			return Candidate.SlotIndex == SlotIndex;
		});

		if (ExistingSlot && ExistingSlot->Tile)
		{
			NewSlot.Tile = ExistingSlot->Tile;
		}
		else
		{
			NewSlot.Tile = NewObject<UHexTile>(this, UHexTile::StaticClass(), NAME_None, RF_Transactional);
			if (NewSlot.Tile)
			{
				NewSlot.Tile->TileType = EHexTileType::Road;
				NewSlot.Tile->Mesh = TileMesh ? TileMesh : (AvailableTileMeshes.Num() > 0 ? AvailableTileMeshes[0] : nullptr);
			}
		}

		TileSlots.Add(NewSlot);
	}
}

int32 AHexGridManager::GetExpectedTileSlotCount() const
{
	const int32 AxialRadius = FMath::Max(0, GridRadius - 1);
	return 1 + 3 * AxialRadius * (AxialRadius + 1);
}

void AHexGridManager::UpdateSlotMetadata()
{
	for (FHexTileSlot& Slot : TileSlots)
	{
		Slot.LocalLocation = GetTileLocation(Slot.Q, Slot.R) - (bCenterGridOnActor ? GetTileLocation(0, 0) : FVector::ZeroVector);
	}
}

void AHexGridManager::AddRingSlots(TArray<FIntPoint>& OutAxialCoordinates, int32 Ring) const
{
	static const FIntPoint ClockwiseDirections[] =
	{
		FIntPoint(1, 0),
		FIntPoint(0, 1),
		FIntPoint(-1, 1),
		FIntPoint(-1, 0),
		FIntPoint(0, -1),
		FIntPoint(1, -1)
	};

	FIntPoint Current(0, -Ring);
	for (const FIntPoint& Direction : ClockwiseDirections)
	{
		for (int32 Step = 0; Step < Ring; ++Step)
		{
			OutAxialCoordinates.Add(Current);
			Current += Direction;
		}
	}
}

FVector2D AHexGridManager::GetTileFootprint() const
{
	const UStaticMesh* StaticMesh = TileMesh;
	if (!StaticMesh)
	{
		for (const FHexTileSlot& Slot : TileSlots)
		{
			if (Slot.Tile && Slot.Tile->Mesh)
			{
				StaticMesh = Slot.Tile->Mesh;
				break;
			}
		}
	}
	if (!StaticMesh && AvailableTileMeshes.Num() > 0)
	{
		StaticMesh = AvailableTileMeshes[0];
	}

	if (!bUseMeshBoundsForTileSpacing || !StaticMesh)
	{
		constexpr float Sqrt3 = 1.7320508075688772f;
		const float SafeTileSize = FMath::Max(TileSize, KINDA_SMALL_NUMBER);

		if (Orientation == EHexGridOrientation::PointyTop)
		{
			return FVector2D(SafeTileSize * Sqrt3, SafeTileSize * 2.0f);
		}

		return FVector2D(SafeTileSize * 2.0f, SafeTileSize * Sqrt3);
	}

	const FBox LocalBounds = StaticMesh->GetBoundingBox();
	const FTransform TileTransform(TileRotation, FVector::ZeroVector, TileScale);
	FBox TransformedBounds(ForceInit);

	for (int32 XIndex = 0; XIndex < 2; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < 2; ++YIndex)
		{
			for (int32 ZIndex = 0; ZIndex < 2; ++ZIndex)
			{
				const FVector Corner(
					XIndex == 0 ? LocalBounds.Min.X : LocalBounds.Max.X,
					YIndex == 0 ? LocalBounds.Min.Y : LocalBounds.Max.Y,
					ZIndex == 0 ? LocalBounds.Min.Z : LocalBounds.Max.Z);

				TransformedBounds += TileTransform.TransformPosition(Corner);
			}
		}
	}

	const FVector Size = TransformedBounds.GetSize();
	if (Size.X > KINDA_SMALL_NUMBER && Size.Y > KINDA_SMALL_NUMBER)
	{
		return FVector2D(Size.X, Size.Y);
	}

	constexpr float Sqrt3 = 1.7320508075688772f;
	const float SafeTileSize = FMath::Max(TileSize, KINDA_SMALL_NUMBER);
	return Orientation == EHexGridOrientation::PointyTop
		? FVector2D(SafeTileSize * Sqrt3, SafeTileSize * 2.0f)
		: FVector2D(SafeTileSize * 2.0f, SafeTileSize * Sqrt3);
}

FVector AHexGridManager::GetTileLocation(int32 Q, int32 R) const
{
	const FVector2D Footprint = GetTileFootprint();

	if (Orientation == EHexGridOrientation::PointyTop)
	{
		const float X = Footprint.X * (static_cast<float>(Q) + static_cast<float>(R) * 0.5f);
		const float Y = Footprint.Y * 0.75f * static_cast<float>(R);
		return FVector(X, Y, 0.0f);
	}

	const float X = Footprint.X * 0.75f * static_cast<float>(Q);
	const float Y = Footprint.Y * (static_cast<float>(R) + static_cast<float>(Q) * 0.5f);
	return FVector(X, Y, 0.0f);
}
