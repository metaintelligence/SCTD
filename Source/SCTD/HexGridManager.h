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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile Slot")
	EHexTileType TileType = EHexTileType::Road;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile Slot", meta = (EditCondition = "TileType == EHexTileType::Road", EditConditionHides))
	bool bEnemySpawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile Slot", meta = (EditCondition = "TileType == EHexTileType::Road", EditConditionHides))
	int32 NextMovementTargetTileIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile Slot")
	TObjectPtr<UStaticMesh> Mesh;
};

UCLASS(Blueprintable)
class SCTD_API AHexGridManager : public AActor
{
	GENERATED_BODY()

public:
	AHexGridManager();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Hex Grid")
	void GenerateGrid();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Hex Grid")
	void ClearGrid();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Hex Grid")
	void RebuildTileSlots();

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	FVector GetTileLocation(int32 Q, int32 R) const;

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	int32 GetExpectedTileSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	bool FindTileSlotAtWorldLocation(const FVector& WorldLocation, FHexTileSlot& OutSlot) const;

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	bool FindTileSlotByIndex(int32 SlotIndex, FHexTileSlot& OutSlot) const;

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	bool GetTileWorldLocationBySlotIndex(int32 SlotIndex, FVector& OutWorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	void GetEnemySpawnTileWorldLocations(TArray<FVector>& OutWorldLocations) const;

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	bool IsWorldLocationAllyTraversable(const FVector& WorldLocation) const;

protected:
	UStaticMesh* GetBoardSizeReferenceMesh() const;
	FVector2D GetTileFootprint() const;
	bool IsLocalPointInsideTileFootprint(const FVector2D& LocalPoint, const FVector& TileCenter) const;
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
