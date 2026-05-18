#include "AbyssSharkAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AbyssDiverCharacter.h" // 플레이어 클래스 확인용
#include "DecoyActor.h"

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
			// 블랙보드에 "TargetActor"라는 이름으로 플레이어를 저장합니다.
			GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), PlayerDiver);

			// 블랙보드에 "HasLineOfSight"를 true로 바꿉니다.
			GetBlackboardComponent()->SetValueAsBool(FName("HasLineOfSight"), true);

			UE_LOG(LogTemp, Warning, TEXT("Shark Find Player"));
		}
		else
		{
			// 시야에서 벗어남 (벽 뒤에 숨거나 멀어짐)
			GetBlackboardComponent()->SetValueAsBool(FName("HasLineOfSight"), false);

			// (선택) 여기서 TargetActor를 nullptr로 만들면 즉시 추적을 포기합니다.
			// GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), nullptr);

			UE_LOG(LogTemp, Warning, TEXT("Shark not found player"));
		}
	}
}