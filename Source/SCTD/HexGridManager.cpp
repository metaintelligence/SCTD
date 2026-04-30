#include "HexGridManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
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
		if (!Slot.Mesh)
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
		TileComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TileComponent->SetStaticMesh(Slot.Mesh);
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
		const int32 SlotIndex = Index;
		FHexTileSlot NewSlot;
		NewSlot.SlotIndex = SlotIndex;
		NewSlot.Q = AxialCoordinates[Index].X;
		NewSlot.R = AxialCoordinates[Index].Y;
		NewSlot.Ring = FMath::Max3(FMath::Abs(NewSlot.Q), FMath::Abs(NewSlot.R), FMath::Abs(-NewSlot.Q - NewSlot.R));
		NewSlot.RingIndex = NewSlot.Ring == 0 ? 0 : Index - (1 + 3 * (NewSlot.Ring - 1) * NewSlot.Ring) + 1;
		NewSlot.LocalLocation = GetTileLocation(NewSlot.Q, NewSlot.R) - CenterOffset;

		const FHexTileSlot* ExistingSlot = ExistingSlots.FindByPredicate([&NewSlot](const FHexTileSlot& Candidate)
		{
			return Candidate.Q == NewSlot.Q && Candidate.R == NewSlot.R;
		});

		if (!ExistingSlot)
		{
			ExistingSlot = ExistingSlots.FindByPredicate([SlotIndex](const FHexTileSlot& Candidate)
			{
				return Candidate.SlotIndex == SlotIndex;
			});
		}

		if (ExistingSlot)
		{
			NewSlot.TileType = ExistingSlot->TileType;
			NewSlot.bEnemySpawn = ExistingSlot->TileType == EHexTileType::Road && ExistingSlot->bEnemySpawn;
			NewSlot.NextMovementTargetTileIndex = ExistingSlot->TileType == EHexTileType::Road ? ExistingSlot->NextMovementTargetTileIndex : INDEX_NONE;
			NewSlot.Mesh = ExistingSlot->Mesh;
		}
		else
		{
			NewSlot.TileType = EHexTileType::Road;
			NewSlot.bEnemySpawn = false;
			NewSlot.NextMovementTargetTileIndex = INDEX_NONE;
			NewSlot.Mesh = TileMesh ? TileMesh : (AvailableTileMeshes.Num() > 0 ? AvailableTileMeshes[0] : nullptr);
		}

		TileSlots.Add(NewSlot);
	}
}

int32 AHexGridManager::GetExpectedTileSlotCount() const
{
	const int32 AxialRadius = FMath::Max(0, GridRadius - 1);
	return 1 + 3 * AxialRadius * (AxialRadius + 1);
}

bool AHexGridManager::FindTileSlotAtWorldLocation(const FVector& WorldLocation, FHexTileSlot& OutSlot) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const FVector2D LocalPoint(LocalLocation.X, LocalLocation.Y);

	const FHexTileSlot* BestSlot = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (const FHexTileSlot& Slot : TileSlots)
	{
		if (!IsLocalPointInsideTileFootprint(LocalPoint, Slot.LocalLocation))
		{
			continue;
		}

		const FVector2D SlotCenter(Slot.LocalLocation.X, Slot.LocalLocation.Y);
		const float DistanceSquared = FVector2D::DistSquared(LocalPoint, SlotCenter);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestSlot = &Slot;
		}
	}

	if (!BestSlot)
	{
		return false;
	}

	OutSlot = *BestSlot;
	return true;
}

bool AHexGridManager::FindTileSlotByIndex(int32 SlotIndex, FHexTileSlot& OutSlot) const
{
	const FHexTileSlot* Slot = TileSlots.FindByPredicate([SlotIndex](const FHexTileSlot& Candidate)
	{
		return Candidate.SlotIndex == SlotIndex;
	});

	if (!Slot)
	{
		return false;
	}

	OutSlot = *Slot;
	return true;
}

bool AHexGridManager::GetTileWorldLocationBySlotIndex(int32 SlotIndex, FVector& OutWorldLocation) const
{
	FHexTileSlot Slot;
	if (!FindTileSlotByIndex(SlotIndex, Slot))
	{
		return false;
	}

	OutWorldLocation = GetActorTransform().TransformPosition(Slot.LocalLocation);
	return true;
}

void AHexGridManager::GetEnemySpawnTileWorldLocations(TArray<FVector>& OutWorldLocations) const
{
	OutWorldLocations.Reset();

	for (const FHexTileSlot& Slot : TileSlots)
	{
		if (!Slot.bEnemySpawn || Slot.TileType != EHexTileType::Road)
		{
			continue;
		}

		OutWorldLocations.Add(GetActorTransform().TransformPosition(Slot.LocalLocation));
	}
}

bool AHexGridManager::IsWorldLocationAllyTraversable(const FVector& WorldLocation) const
{
	FHexTileSlot Slot;
	if (!FindTileSlotAtWorldLocation(WorldLocation, Slot))
	{
		return false;
	}

	return Slot.TileType != EHexTileType::Block;
}

bool AHexGridManager::IsLocalPointInsideTileFootprint(const FVector2D& LocalPoint, const FVector& TileCenter) const
{
	const FVector2D Footprint = GetTileFootprint();
	const float HalfWidth = Footprint.X * 0.5f;
	const float HalfHeight = Footprint.Y * 0.5f;
	if (HalfWidth <= KINDA_SMALL_NUMBER || HalfHeight <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float DeltaX = FMath::Abs(LocalPoint.X - TileCenter.X);
	const float DeltaY = FMath::Abs(LocalPoint.Y - TileCenter.Y);

	if (DeltaX > HalfWidth || DeltaY > HalfHeight)
	{
		return false;
	}

	if (Orientation == EHexGridOrientation::PointyTop)
	{
		return DeltaY / HalfHeight + DeltaX / Footprint.X <= 1.0f;
	}

	return DeltaX / HalfWidth + DeltaY / Footprint.Y <= 1.0f;
}

void AHexGridManager::UpdateSlotMetadata()
{
	for (FHexTileSlot& Slot : TileSlots)
	{
		if (Slot.TileType != EHexTileType::Road)
		{
			Slot.bEnemySpawn = false;
			Slot.NextMovementTargetTileIndex = INDEX_NONE;
		}

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

UStaticMesh* AHexGridManager::GetBoardSizeReferenceMesh() const
{
	if (AvailableTileMeshes.Num() > 0 && AvailableTileMeshes[0])
	{
		return AvailableTileMeshes[0];
	}

	return TileMesh;
}

FVector2D AHexGridManager::GetTileFootprint() const
{
	const UStaticMesh* StaticMesh = GetBoardSizeReferenceMesh();

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
