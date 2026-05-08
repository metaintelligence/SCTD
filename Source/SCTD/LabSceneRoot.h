#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabSceneRoot.generated.h"

UCLASS(Blueprintable)
class SCTD_API ALabSceneRoot : public AActor
{
	GENERATED_BODY()

public:
	ALabSceneRoot();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LAB")
	bool bShowMouseCursor = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<class ULabTurretFusionWidget> LabWidget;

	UPROPERTY(Transient)
	TObjectPtr<class USCTDUserRepository> UserRepository;

	void EnsureLabWidget();
	void SeedMockPartsIfNeeded();
};
