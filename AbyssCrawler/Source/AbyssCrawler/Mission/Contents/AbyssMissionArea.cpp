// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionArea.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbyssDiverCharacter.h"
#include "AbyssGameMode.h"

// Sets default values
AAbyssMissionArea::AAbyssMissionArea()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 150.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

}

// Called when the game starts or when spawned
void AAbyssMissionArea::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(
		this,
		&AAbyssMissionArea::OnTriggerBeginOverlap
	);
	
}

void AAbyssMissionArea::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (bTriggered) return;

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(OtherActor);
	if (!Diver) return;

	bTriggered = true;

	if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
	{
		GM->AddMissionProgress(MissionIndex, 1);
	}

	UE_LOG(LogTemp, Warning, TEXT("[MissionArea] Reached Area MissionIndex=%d"), MissionIndex);

	// 한 번 사용 후 종료
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AAbyssMissionArea::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssMissionArea, bTriggered);
}


