// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionItem.h"
#include "AbyssGameMode.h"
#include "AbyssDiverCharacter.h"
#include "AbyssGameState.h"

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

    AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
    if (!GS) return;

    // 해당 미션을 안 받았으면 작동 X
    if (!GS->HasActiveMission(MissionId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionItem] Mission not active: %s"), *MissionId.ToString());
        return;
    }

    if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
    {
        GM->AddMissionProgressById(MissionId, 1);
    }

    if (bDestroyOnCollect)
    {
        Destroy();
    }

}

