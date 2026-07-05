#include "AbyssOctopusAIController.h"
#include "AbyssOctopusCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "DecoyActor.h"
#include "TimerManager.h"

AAbyssOctopusAIController::AAbyssOctopusAIController()
{
	OctopusPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("OctopusPerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 900.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	OctopusPerceptionComp->ConfigureSense(*SightConfig);
	OctopusPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	//OctopusPerceptionComp->SetDominantSense(SightConfig->GetSenseID());
	OctopusPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAbyssOctopusAIController::OnTargetDetected);
}

void AAbyssOctopusAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AIBehavior)
	{
		RunBehaviorTree(AIBehavior);
	}

	// 둥지(스폰) 위치 기록 + 블랙보드에 전달 (영역 리쉬/복귀용)
	if (InPawn)
	{
		HomeLocation = InPawn->GetActorLocation();
		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->SetValueAsVector(HomeLocationKey, HomeLocation);
		}
	}

	// 주기적 영역 리쉬 검사 타이머 시작
	GetWorldTimerManager().SetTimer(
		LeashCheckTimerHandle, this, &AAbyssOctopusAIController::CheckHomeLeash,
		LeashCheckInterval, /*bLoop=*/true);
}

void AAbyssOctopusAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	if (GetBlackboardComponent())
	{
		AActor* CurrentTarget = Cast<AActor>(GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")));
		if (CurrentTarget && CurrentTarget->IsA(ADecoyActor::StaticClass()))
		{
			return; // Do not override target if currently chasing decoy
		}
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// Detected someone, disable stealth
		if (AAbyssOctopusCharacter* Octopus = Cast<AAbyssOctopusCharacter>(GetPawn()))
		{
			Octopus->SetStealthMode(false);
		}

		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->SetValueAsObject(TargetActorKey, Actor);
		}

		// 재발견 → 진행 중이던 흥미 상실(추적 포기) 타이머 취소
		GetWorldTimerManager().ClearTimer(GiveUpTimerHandle);
	}
	else
	{
		// 1단계: 시야를 놓침 → 즉시 포기하지 않고 마지막 목격 위치를 기록하고
		// 일정 시간(LoseInterestTime) 수색하다가 타이머 만료 시 추적 포기 + 은신 복귀.
		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->SetValueAsVector(LastKnownLocationKey, Stimulus.StimulusLocation);
		}

		GetWorldTimerManager().SetTimer(
			GiveUpTimerHandle, this, &AAbyssOctopusAIController::OnGiveUpChase,
			LoseInterestTime, /*bLoop=*/false);
	}
}

void AAbyssOctopusAIController::OnGiveUpChase()
{
	// 흥미 상실 타이머 만료: 수색해도 다시 못 봤으므로 추적을 완전히 포기.
	AbandonChase();
}

void AAbyssOctopusAIController::CheckHomeLeash()
{
	// 3단계: 둥지에서 너무 멀어지면 추적 포기 후 복귀.
	APawn* MyPawn = GetPawn();
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!MyPawn || !BB)
	{
		return;
	}

	// 현재 추적 대상이 없으면 검사할 필요 없음.
	if (!BB->GetValueAsObject(TargetActorKey))
	{
		return;
	}

	const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), HomeLocation);
	if (DistSq > FMath::Square(HomeLeashRadius))
	{
		AbandonChase();
	}
}

void AAbyssOctopusAIController::AbandonChase()
{
	GetWorldTimerManager().ClearTimer(GiveUpTimerHandle);

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->ClearValue(TargetActorKey);
		// BT가 둥지로 복귀하도록 HomeLocation을 최신화.
		BB->SetValueAsVector(HomeLocationKey, HomeLocation);
	}

	// 추적을 포기하면 다시 은신 모드로.
	if (AAbyssOctopusCharacter* Octopus = Cast<AAbyssOctopusCharacter>(GetPawn()))
	{
		Octopus->SetStealthMode(true);
	}
}
