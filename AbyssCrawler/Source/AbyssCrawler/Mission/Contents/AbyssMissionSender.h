// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssGameState.h"
#include "AbyssMissionSender.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionSender : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionSender();

	virtual void Interact_Implementation(AActor* InstigatorActor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TArray<FAbyssMissionData> AvailableMissions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bGiveAllAtOnce = true;
};
