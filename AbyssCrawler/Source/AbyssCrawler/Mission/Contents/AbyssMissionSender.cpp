// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionSender.h"
#include "Components/StaticMeshComponent.h"
#include "AbyssGameMode.h"
#include "AbyssGameState.h"
#include "AbyssDiverCharacter.h"
#include "Net/UnrealNetwork.h"

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

	// 다른 플레이어가 사용 중이면 거부
	if (bIsInUse && UsingCharacter != Diver)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MissionSender] Already in use"));
		return;
	}

	bIsInUse = true;
	UsingCharacter = Diver;

	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
	if (!GS) return;

	TArray<FAbyssMissionData> FilteredMissions = GS->GetAvailableMissions();

	Diver->Client_OpenMissionSelectUI(FilteredMissions, this);
}

void AAbyssMissionSender::ReleaseMissionSender(AAbyssDiverCharacter* Character)
{
	if (!HasAuthority()) return;

	if (UsingCharacter != Character)
	{
		return;
	}

	bIsInUse = false;
	UsingCharacter = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[MissionSender] Released"));
}

void AAbyssMissionSender::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssMissionSender, bIsInUse);
}


