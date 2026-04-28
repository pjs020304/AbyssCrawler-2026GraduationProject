// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionWorkObject.h"
#include "Components/StaticMeshComponent.h"
#include "AbyssDiverCharacter.h"
#include "AbyssGameMode.h"

AAbyssMissionWorkObject::AAbyssMissionWorkObject()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

}

void AAbyssMissionWorkObject::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;
	if (bCompleted) return;
	if (bIsWorking) return;

	WorkingCharacter = Cast<AAbyssDiverCharacter>(InstigatorActor);
	if (!WorkingCharacter) return;

	bIsWorking = true;

	CurrentWorkTime = 0.0f;

	WorkingCharacter->Client_ShowWorkUI();
	WorkingCharacter->Client_UpdateWorkProgress(0.0f);

	GetWorldTimerManager().SetTimer(
		WorkTimerHandle,
		this,
		&AAbyssMissionWorkObject::CompleteWork,
		WorkDuration,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("[MissionWork] Work Started"));
}

void AAbyssMissionWorkObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()) return;
	if (!bIsWorking || bCompleted) return;
	if (!WorkingCharacter) return;

	CurrentWorkTime += DeltaTime;

	const float Progress = FMath::Clamp(CurrentWorkTime / WorkDuration, 0.0f, 1.0f);

	WorkingCharacter->Client_UpdateWorkProgress(Progress);
}


void AAbyssMissionWorkObject::CompleteWork()
{
	if (!HasAuthority()) return;

	bIsWorking = false;
	bCompleted = true;

	if (WorkingCharacter)
	{
		WorkingCharacter->Client_UpdateWorkProgress(1.0f);
		WorkingCharacter->Client_HideWorkUI();
	}

	if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
	{
		GM->AddMissionProgress(MissionIndex, 1);
	}

	UE_LOG(LogTemp, Warning, TEXT("[MissionWork] Work Completed MissionIndex=%d"), MissionIndex);
}


