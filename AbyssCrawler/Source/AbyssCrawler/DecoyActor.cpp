#include "DecoyActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbyssEnemyBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADecoyActor::ADecoyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = SphereComp;
	SphereComp->InitSphereRadius(50.0f);
	SphereComp->SetCollisionProfileName(TEXT("OverlapAll"));

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);

	bReplicates = true;
}

void ADecoyActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SetLifeSpan(DecoyLifespan);
		GetWorldTimerManager().SetTimer(AttractionTimerHandle, this, &ADecoyActor::AttractEnemies, 0.5f, true);
	}
}

void ADecoyActor::AttractEnemies()
{
	TArray<AActor*> OverlappingActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAbyssEnemyBase::StaticClass(), OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		AAbyssEnemyBase* Enemy = Cast<AAbyssEnemyBase>(Actor);
		if (Enemy && FVector::Dist(GetActorLocation(), Enemy->GetActorLocation()) <= AttractionRadius)
		{
			AAIController* AICon = Cast<AAIController>(Enemy->GetController());
			if (AICon && AICon->GetBlackboardComponent())
			{
				AICon->GetBlackboardComponent()->SetValueAsObject("TargetActor", this);
				AICon->GetBlackboardComponent()->SetValueAsBool("HasLineOfSight", true);
			}
		}
	}
}
