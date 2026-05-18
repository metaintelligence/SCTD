#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventorySceneRoot.generated.h"

UCLASS(Blueprintable)
class SCTD_API AInventorySceneRoot : public AActor
{
	GENERATED_BODY()

public:
	AInventorySceneRoot();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bShowMouseCursor = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UInventoryWidget> InventoryWidget;

	UPROPERTY(Transient)
	TObjectPtr<class USCTDUserRepository> UserRepository;

	void EnsureInventoryWidget();
};
