// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssStartLightController.generated.h"

class ADirectionalLight;
class ASkyLight;

UCLASS()
class ABYSSCRAWLER_API AAbyssStartLightController : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbyssStartLightController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, Category = "Light")
	ADirectionalLight* DirectionalLight;

	UPROPERTY(EditAnywhere, Category = "Light")
	ASkyLight* SkyLight;

	UPROPERTY(EditAnywhere, Category = "Light")
	float DelayTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Light")
	float FadeTime = 3.0f;

	float ElapsedTime = 0.0f;

	float OriginalDirectionalIntensity = 0.0f;
	float OriginalSkyIntensity = 0.0f;

	bool bStartFade = false;

};
