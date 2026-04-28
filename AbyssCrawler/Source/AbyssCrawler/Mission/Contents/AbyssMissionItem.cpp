// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionItem.h"
#include "AbyssGameMode.h"
#include "AbyssDiverCharacter.h"


AAbyssMissionItem::AAbyssMissionItem()
{
    bReplicates = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

}

void AAbyssMissionItem::Interact_Implementation(AActor* InstigatorActor)
{
    if (!HasAuthority()) return;

    AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
    if (!Diver) return;

    if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
    {
        GM->OnMissionItemCollected(MissionIndex);
    }
    Destroy();

}

