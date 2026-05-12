// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssDiverCharacter.h"
#include "AbyssMissionWorkObject.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;
class AAbyssSharkCharacter;

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionWorkObject : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionWorkObject();

	virtual void Interact_Implementation(AActor* InstigatorActor) override;

	virtual void Tick(float DeltaTime) override;

	void CancelWork();

	float CurrentWorkTime = 0.0f;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 MissionIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	float WorkDuration = 5.0f;

	UPROPERTY()
	AAbyssDiverCharacter* WorkingCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	float EnemyCancelRadius = 600.0f;

	FTimerHandle WorkTimerHandle;

	bool bIsWorking = false;
	bool bCompleted = false;

	void CompleteWork();

	bool IsEnemyNearWorkingCharacter() const;
};
