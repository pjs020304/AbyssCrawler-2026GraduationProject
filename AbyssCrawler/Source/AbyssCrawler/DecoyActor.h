#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DecoyActor.generated.h"

UCLASS()
class ABYSSCRAWLER_API ADecoyActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ADecoyActor();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* SphereComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decoy")
	float AttractionRadius = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decoy")
	float DecoyLifespan = 10.0f;

protected:
	FTimerHandle AttractionTimerHandle;

	UFUNCTION()
	void AttractEnemies();
};
