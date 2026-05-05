// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionSender.h"
#include "Components/StaticMeshComponent.h"
#include "AbyssGameMode.h"
#include "AbyssGameState.h"
#include "AbyssDiverCharacter.h"

AAbyssMissionSender::AAbyssMissionSender()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

void AAbyssMissionSender::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
	if (!Diver) return;

	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
	if (!GS) return;

	TArray<FAbyssMissionData> FilteredMissions = GS->GetAvailableMissions();

	Diver->Client_OpenMissionSelectUI(FilteredMissions);
}


