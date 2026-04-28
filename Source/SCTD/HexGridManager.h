#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexGridManager.generated.h"

class UInstancedStaticMeshComponent;

UENUM(BlueprintType)
enum class EHexGridOrientation : uint8
{
	FlatTop UMETA(DisplayName = "Flat Top"),
	PointyTop UMETA(DisplayName = "Pointy Top")
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

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	FVector GetTileLocation(int32 Q, int32 R) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> TileInstances;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid", meta = (ClampMin = "0"))
	int32 GridRadius = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid", meta = (ClampMin = "1.0"))
	float TileSize = 100.0f;

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
};
