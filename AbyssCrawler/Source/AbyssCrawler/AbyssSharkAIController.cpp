#include "AbyssSharkAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AbyssDiverCharacter.h" // 플레이어 클래스 확인용
#include "DecoyActor.h"
#include "TimerManager.h"

AAbyssSharkAIController::AAbyssSharkAIController()
{
	// 1. Perception 컴포넌트 생성
	SharkPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("SharkPerceptionComp"));

	// 2. 시각(Sight) 설정 객체 생성
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	if (SightConfig)
	{
		// 시야 거리 (예: 3000 = 30미터 밖에서 감지 가능)
		SightConfig->SightRadius = 3000.0f;

		// 시야에서 사라졌다고 판단하는 거리 (살짝 더 멀게 설정하여 놓치는 것을 지연)
		SightConfig->LoseSightRadius = 3500.0f;

		// 시야각 (양옆으로 90도씩, 총 180도)
		SightConfig->PeripheralVisionAngleDegrees = 90.0f;

		// 감지할 대상 설정 (적, 중립, 아군 모두 감지)
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		// 설정 완료 후 컴포넌트에 시각 감지 등록
		SharkPerceptionComp->ConfigureSense(*SightConfig);
		SharkPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}

void AAbyssSharkAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 1. 빙의 시 시각 감지 이벤트 바인딩
	if (SharkPerceptionComp)
	{
		SharkPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAbyssSharkAIController::OnTargetDetected);
	}

	// 2. 행동 트리(Behavior Tree)가 설정되어 있다면 실행!
	if (AIBehavior)
	{
		RunBehaviorTree(AIBehavior);
	}

	// 3. 둥지(스폰) 위치 기록 + 블랙보드에 전달 (영역 리쉬/복귀용)
	if (InPawn)
	{
		HomeLocation = InPawn->GetActorLocation();
		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->SetValueAsVector(HomeLocationKey, HomeLocation);
		}
	}

	// 4. 주기적 영역 리쉬 검사 타이머 시작
	GetWorldTimerManager().SetTimer(
		LeashCheckTimerHandle, this, &AAbyssSharkAIController::CheckHomeLeash,
		LeashCheckInterval, /*bLoop=*/true);
}

// 무언가를 보거나(True), 시야에서 놓쳤을 때(False) 엔진이 자동으로 호출해줍니다.
void AAbyssSharkAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	if (GetBlackboardComponent())
	{
		AActor* CurrentTarget = Cast<AActor>(GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")));
		if (CurrentTarget && CurrentTarget->IsA(ADecoyActor::StaticClass()))
		{
			return; // Do not override target if currently chasing decoy
		}
	}

	// 1. 감지한 액터가 우리가 찾는 플레이어(AbyssDiverCharacter)인지 확인
	AAbyssDiverCharacter* PlayerDiver = Cast<AAbyssDiverCharacter>(Actor);

	if (PlayerDiver && GetBlackboardComponent())
	{
		// 2. 시야에 들어왔는가? (Stimulus.WasSuccessfullySensed()가 true면 봄, false면 놓침)
		if (Stimulus.WasSuccessfullySensed())
		{
			// 플레이어를 타겟으로 저장 + 시야 확보
			GetBlackboardComponent()->SetValueAsObject(TargetActorKey, PlayerDiver);
			GetBlackboardComponent()->SetValueAsBool(HasLineOfSightKey, true);

			// 재발견 → 진행 중이던 흥미 상실(추적 포기) 타이머 취소
			GetWorldTimerManager().ClearTimer(GiveUpTimerHandle);

			UE_LOG(LogTemp, Warning, TEXT("Shark Find Player"));
		}
		else
		{
			// 1단계: 시야에서 벗어남 → 즉시 포기하지 않고 마지막 목격 위치를 기록하고
			// 일정 시간(LoseInterestTime) 동안 수색하다가 타이머 만료 시 추적 포기.
			GetBlackboardComponent()->SetValueAsBool(HasLineOfSightKey, false);
			GetBlackboardComponent()->SetValueAsVector(LastKnownLocationKey, Stimulus.StimulusLocation);

			GetWorldTimerManager().SetTimer(
				GiveUpTimerHandle, this, &AAbyssSharkAIController::OnGiveUpChase,
				LoseInterestTime, /*bLoop=*/false);

			UE_LOG(LogTemp, Warning, TEXT("Shark lost sight - investigating for %.1fs"), LoseInterestTime);
		}
	}
}

void AAbyssSharkAIController::OnGiveUpChase()
{
	// 흥미 상실 타이머 만료: 수색해도 다시 못 봤으므로 추적을 완전히 포기.
	AbandonChase();
	UE_LOG(LogTemp, Warning, TEXT("Shark gave up chase (lost interest)"));
}

void AAbyssSharkAIController::CheckHomeLeash()
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
		UE_LOG(LogTemp, Warning, TEXT("Shark gave up chase (home leash exceeded)"));
	}
}

void AAbyssSharkAIController::AbandonChase()
{
	GetWorldTimerManager().ClearTimer(GiveUpTimerHandle);

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->ClearValue(TargetActorKey);
		BB->SetValueAsBool(HasLineOfSightKey, false);
		// BT가 둥지로 복귀하도록 HomeLocation을 최신화.
		BB->SetValueAsVector(HomeLocationKey, HomeLocation);
	}
}