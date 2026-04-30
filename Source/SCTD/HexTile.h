#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HexTile.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EHexTileType : uint8
{
	Road UMETA(DisplayName = "Road"),
	Block UMETA(DisplayName = "Block"),
	Resource UMETA(DisplayName = "Resource"),
	Tower UMETA(DisplayName = "Tower")
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCTD_API UHexTile : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	EHexTileType TileType = EHexTileType::Road;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Tile")
	TObjectPtr<UStaticMesh> Mesh;

	UFUNCTION(BlueprintPure, Category = "Hex Tile")
	bool CanEnemyTraverse() const;

	UFUNCTION(BlueprintPure, Category = "Hex Tile")
	bool CanAllyTraverse() const;

	UFUNCTION(BlueprintPure, Category = "Hex Tile")
	bool CanBuildTower() const;
};
