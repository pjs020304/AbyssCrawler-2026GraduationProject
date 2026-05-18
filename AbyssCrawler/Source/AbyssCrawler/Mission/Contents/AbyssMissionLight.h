// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssMissionLight.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionLight : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionLight();

	void SetLightsEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light")
	TArray<AActor*> LightActors;

	UPROPERTY(ReplicatedUsing = OnRep_LightsOn)
	bool bLightsOn = false;

	UFUNCTION()
	void OnRep_LightsOn();

	virtual void BeginPlay() override;
};
