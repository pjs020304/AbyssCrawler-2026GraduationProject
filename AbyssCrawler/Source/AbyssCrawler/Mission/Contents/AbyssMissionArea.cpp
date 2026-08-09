// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionArea.h"

#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbyssDiverCharacter.h"
#include "AbyssGameMode.h"
#include "GameFramework/PlayerController.h"

AAbyssMissionArea::AAbyssMissionArea()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	AreaCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaCollision"));
	AreaCollision->SetupAttachment(RootComponent);

	AreaCollision->SetBoxExtent(AreaExtent);
	AreaCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AreaCollision->SetGenerateOverlapEvents(true);

	MissionArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissionArrowMesh"));
	MissionArrowMesh->SetupAttachment(RootComponent);
	MissionArrowMesh->SetRelativeLocation(ArrowOffset);
	MissionArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MissionArrowMesh->SetVisibility(false);
	MissionArrowMesh->SetHiddenInGame(true);

	MissionMarkerWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("MissionMarkerWidget"));
	MissionMarkerWidget->SetupAttachment(RootComponent);

	MissionMarkerWidget->SetWidgetSpace(EWidgetSpace::Screen);
	MissionMarkerWidget->SetDrawAtDesiredSize(true);
	MissionMarkerWidget->SetRelativeLocation(MarkerOffset);

	MissionMarkerWidget->SetVisibility(false);
	MissionMarkerWidget->SetHiddenInGame(true);
}

void AAbyssMissionArea::BeginPlay()
{
	Super::BeginPlay();

	if (AreaCollision)
	{
		AreaCollision->SetBoxExtent(AreaExtent);
		AreaCollision->OnComponentBeginOverlap.AddDynamic(
			this,
			&AAbyssMissionArea::OnAreaBeginOverlap
		);
	}

	if (MissionArrowMesh)
	{
		MissionArrowMesh->SetRelativeLocation(ArrowOffset);
		ArrowBaseLocation = ArrowOffset;
	}

	if (MissionMarkerWidget)
	{
		MissionMarkerWidget->SetRelativeLocation(MarkerOffset);
	}

	ApplyMarkerVisible();
}

void AAbyssMissionArea::SetMissionMarkerVisible(bool bVisible)
{
	if (!HasAuthority())
	{
		return;
	}

	bMissionMarkerVisible = bVisible;

	if (bVisible)
	{
		bTriggered = false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[MissionArea] SetMarkerVisible %s / MissionId=%s"),
		bVisible ? TEXT("true") : TEXT("false"),
		*MissionId.ToString()
	);

	ApplyMarkerVisible();
}

void AAbyssMissionArea::OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bTriggered)
	{
		return;
	}

	if (!bMissionMarkerVisible)
	{
		return;
	}

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(OtherActor);
	if (!Diver)
	{
		return;
	}

	if (MissionId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MissionArea] MissionId is None"));
		return;
	}

	bTriggered = true;

	if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
	{
		GM->AddMissionProgressById(MissionId, 1);
	}

	UE_LOG(LogTemp, Warning, TEXT("[MissionArea] Reached Area MissionId=%s"),
		*MissionId.ToString()
	);

	ApplyMarkerVisible();
}


void AAbyssMissionArea::OnRep_MissionMarkerVisible()
{
	ApplyMarkerVisible();
}

void AAbyssMissionArea::ApplyMarkerVisible()
{
	const bool bShouldShow = bMissionMarkerVisible && !bTriggered;

	if (MissionMarkerWidget)
	{
		MissionMarkerWidget->SetVisibility(false);
		MissionMarkerWidget->SetHiddenInGame(true);
	}

	if (MissionArrowMesh)
	{
		MissionArrowMesh->SetVisibility(bShouldShow);
		MissionArrowMesh->SetHiddenInGame(!bShouldShow);
	}

	if (AreaCollision)
	{
		AreaCollision->SetCollisionEnabled(
			bShouldShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision
		);
	}
}

void AAbyssMissionArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!MissionArrowMesh)
	{
		return;
	}

	if (!bMissionMarkerVisible || bTriggered)
	{
		MissionArrowMesh->SetVisibility(false);
		MissionArrowMesh->SetHiddenInGame(true);
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	const float Distance = FVector::Dist(
		PC->GetPawn()->GetActorLocation(),
		GetActorLocation()
	);

	const bool bTooClose = Distance <= ArrowHideDistance;

	MissionArrowMesh->SetVisibility(!bTooClose);
	MissionArrowMesh->SetHiddenInGame(bTooClose);

	if (bTooClose)
	{
		return;
	}

	const float Time = GetWorld()->GetTimeSeconds();

	FVector NewLocation = ArrowBaseLocation;
	NewLocation.Z += FMath::Sin(Time * ArrowBobSpeed) * ArrowBobAmplitude;

	MissionArrowMesh->SetRelativeLocation(NewLocation);
}

void AAbyssMissionArea::UpdateMarkerDistanceVisibility()
{
	if (!MissionMarkerWidget)
	{
		return;
	}

	// 미션이 활성화되지 않았거나 이미 도착했다면 숨김
	if (!bMissionMarkerVisible || bTriggered)
	{
		MissionMarkerWidget->SetVisibility(false);
		MissionMarkerWidget->SetHiddenInGame(true);
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	APawn* LocalPawn = PC->GetPawn();
	if (!LocalPawn)
	{
		return;
	}

	float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(),GetActorLocation());

	if (AreaCollision)
	{
		FVector ClosestPoint;
		const float ClosestDistance = AreaCollision->GetClosestPointOnCollision(
			LocalPawn->GetActorLocation(), ClosestPoint);

		if (ClosestDistance >= 0.0f)
		{
			Distance = FVector::Dist(LocalPawn->GetActorLocation(), ClosestPoint);
		}
	}

	const bool bTooClose = Distance <= MarkerHideDistance;
	const bool bShouldShow = !bTooClose;

	MissionMarkerWidget->SetVisibility(bShouldShow);
	MissionMarkerWidget->SetHiddenInGame(!bShouldShow);
}

void AAbyssMissionArea::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssMissionArea, bMissionMarkerVisible);
	DOREPLIFETIME(AAbyssMissionArea, bTriggered);
}
