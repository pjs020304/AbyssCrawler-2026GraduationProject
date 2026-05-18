// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionLight.h"
#include "Components/LightComponent.h"
#include "Net/UnrealNetwork.h"

AAbyssMissionLight::AAbyssMissionLight()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AAbyssMissionLight::SetLightsEnabled(bool bEnabled)
{
	bLightsOn = bEnabled;

	for (AActor* LightActor : LightActors)
	{
		if (!LightActor) continue;

		TArray<ULightComponent*> LightComponents;
		LightActor->GetComponents<ULightComponent>(LightComponents);

		for (ULightComponent* LightComp : LightComponents)
		{
			if (LightComp)
			{
				LightComp->SetVisibility(bEnabled);
			}
		}
	}
}

void AAbyssMissionLight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssMissionLight, bLightsOn);
}

void AAbyssMissionLight::OnRep_LightsOn()
{
	SetLightsEnabled(bLightsOn);
}

void AAbyssMissionLight::BeginPlay()
{
	Super::BeginPlay();

	SetLightsEnabled(false);
}



