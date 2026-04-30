#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexTile.h"
#include "HexGridManager.generated.h"

class UStaticMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EHexGridOrientation : uint8
{
	FlatTop UMETA(DisplayName = "Flat Top"),
	PointyTop UMETA(DisplayName = "Pointy Top")
};

USTRUCT(BlueprintType)
struct FHexTileSlot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile Slot")
	int32 SlotIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile Slot")
	int32 Ring = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile Slot")
	int32 RingIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile Slot")
	int32 Q = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile Slot")
	int32 R = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Tile Slot")
	FVector LocalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Hex Tile Slot")
	TObjectPtr<UHexTile> Tile;
};

UCLASS(Blueprintable)
class SCTD_API AHexGridManager : public AActor
{
	GENERATED_BODY()

public:
	AHexGridManager();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateGrid();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ClearGrid();

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void RebuildTileSlots();

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	FVector GetTileLocation(int32 Q, int32 R) const;

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	int32 GetExpectedTileSlotCount() const;

protected:
	FVector2D GetTileFootprint() const;
	void UpdateSlotMetadata();
	void AddRingSlots(TArray<FIntPoint>& OutAxialCoordinates, int32 Ring) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid", meta = (ClampMin = "1"))
	int32 GridRadius = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	TArray<FHexTileSlot> TileSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	TArray<TObjectPtr<UStaticMesh>> AvailableTileMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	bool bUseMeshBoundsForTileSpacing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid", meta = (ClampMin = "1.0", EditCondition = "!bUseMeshBoundsForTileSpacing"))
	float TileSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	TObjectPtr<UStaticMesh> TileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	EHexGridOrientation Orientation = EHexGridOrientation::FlatTop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	bool bGenerateOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	bool bCenterGridOnActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	FRotator TileRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	FVector TileScale = FVector::OneVector;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> GeneratedTileComponents;
};
