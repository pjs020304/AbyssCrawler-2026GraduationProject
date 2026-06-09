// Fill out your copyright notice in the Description page of Project Settings.


#include "AbyssStartLightController.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"

AAbyssStartLightController::AAbyssStartLightController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAbyssStartLightController::BeginPlay()
{
	Super::BeginPlay();

	if (DirectionalLight)
	{
		OriginalDirectionalIntensity =
			DirectionalLight->GetLightComponent()->Intensity;

		DirectionalLight->GetLightComponent()->SetIntensity(0.f);
	}

	if (SkyLight)
	{
		OriginalSkyIntensity =
			SkyLight->GetLightComponent()->Intensity;

		SkyLight->GetLightComponent()->SetIntensity(0.f);
	}
}

void AAbyssStartLightController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	if (!bStartFade)
	{
		if (ElapsedTime >= DelayTime)
		{
			bStartFade = true;
			ElapsedTime = 0.f;
		}

		return;
	}

	float Alpha = FMath::Clamp(
		ElapsedTime / FadeTime,
		0.f,
		1.f);

	if (DirectionalLight)
	{
		DirectionalLight->GetLightComponent()->SetIntensity(
			FMath::Lerp(
				0.f,
				OriginalDirectionalIntensity,
				Alpha));
	}

	if (SkyLight)
	{
		SkyLight->GetLightComponent()->SetIntensity(
			FMath::Lerp(
				0.f,
				OriginalSkyIntensity,
				Alpha));
	}

	if (Alpha >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}
