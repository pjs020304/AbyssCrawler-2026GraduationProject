#include "BTTask_OctopusAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbyssOctopusCharacter.h"
#include "AbyssDiverCharacter.h"

UBTTask_OctopusAttack::UBTTask_OctopusAttack()
{
	NodeName = TEXT("Octopus Grab Attack");

	// 기본 공격 사거리 (GASAttack의 Range와 유사하게 설정)
	AttackRange = 300.0f;
}

EBTNodeResult::Type UBTTask_OctopusAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 1. 컨트롤러 및 폰(문어) 유효성 검사
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	AAbyssOctopusCharacter* Octopus = Cast<AAbyssOctopusCharacter>(AIController->GetPawn());
	if (!Octopus) return EBTNodeResult::Failed;

	if (!Octopus->CanGrab())
	{
		return EBTNodeResult::Failed;
	}

	// 2. 블랙보드에서 타겟(플레이어) 가져오기
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		AAbyssDiverCharacter* TargetActor = Cast<AAbyssDiverCharacter>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));

		if (!TargetActor) {
			return EBTNodeResult::Failed;
		}

		// 거리 판별 로직
		FVector OctopusLocation = Octopus->GetActorLocation();
		FVector TargetLocation = TargetActor->GetActorLocation();

		float DistanceToTarget = FVector::Dist(OctopusLocation, TargetLocation);

		// 공격 사거리를 벗어났다면?
		if (DistanceToTarget > AttackRange)
		{
			UE_LOG(LogTemp, Warning, TEXT("Octopus is too far to grab! Distance: %f, Range: %f"), DistanceToTarget, AttackRange);
			return EBTNodeResult::Failed;
		}

		// 3. Octopus의 GrabPlayer 직접 호출
		Octopus->GrabPlayer(TargetActor);

		// 4. 작업 완료 반환
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
