// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssMissionItem.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionItem : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionItem();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 MissionIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FText ItemDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bDestroyOnCollect = true;

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

public:	
	virtual void Interact_Implementation(AActor* InstigatorActor) override;

};
