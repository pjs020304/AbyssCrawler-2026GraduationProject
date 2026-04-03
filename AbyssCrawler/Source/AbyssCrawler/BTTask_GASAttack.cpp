#include "BTTask_GASAttack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"

// GAS 관련 필수 헤더
#include "AbilitySystemBlueprintLibrary.h" 
#include "AbilitySystemComponent.h"

UBTTask_GASAttack::UBTTask_GASAttack()
{
	NodeName = TEXT("GAS Attack (Event Driven)");

	// 기본 공격 사거리를 설정 (2.5미터)
	AttackRange = 600.0f;
}

EBTNodeResult::Type UBTTask_GASAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 1. 컨트롤러와 폰(상어) 유효성 검사
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* Shark = AIController->GetPawn();
	if (!Shark) return EBTNodeResult::Failed;

	// 2. 상어의 Ability System Component (ASC)를 안전하게 가져옵니다.
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Shark);
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("Shark does not have an Ability System Component!"));
		return EBTNodeResult::Failed;
	}

	// 3. 알림(Event) 방식의 핵심: 어빌리티에 넘겨줄 '데이터 페이로드' 생성
	FGameplayEventData Payload;
	Payload.EventTag = AttackAbilityTag; // 헤더에서 지정한 태그
	Payload.Instigator = Shark;          // 공격의 주체 (상어)

	// 헤더 파일에 TargetKey(FBlackboardKeySelector)를 추가해 target을 넘겨줌.
	
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));

		if (!TargetActor) {
			return EBTNodeResult::Failed;
		}

		// 거리 판별 로직 
		FVector SharkLocation = Shark->GetActorLocation();
		FVector TargetLocation = TargetActor->GetActorLocation();

		// 상어와 플레이어 사이의 실제 3D 거리를 계산합니다.
		float DistanceToTarget = FVector::Dist(SharkLocation, TargetLocation);

		// 설정한 공격 사거리보다 멀다면?
		if (DistanceToTarget > AttackRange)
		{
			UE_LOG(LogTemp, Warning, TEXT("Shark is too far to bite! Distance: %f, Range: %f"), DistanceToTarget, AttackRange);

			// 이벤트를 쏘지 않고 태스크를 실패 상태로 종료
			// BT는 Selector에 의해 다음 프레임에 다시 추적(MoveTo) 태스크를 실행
			return EBTNodeResult::Failed;
		}
		


		if (TargetActor)
		{
			Payload.Target = TargetActor; // 편지에 대상을 명확히 기재
			UE_LOG(LogTemp, Warning, TEXT("Payload Event Send"), *TargetActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("TargetActor is null! Check The BlackBoardKey"));
		}
	}

	// 4. 이벤트 발송 (알림)
	// 상어 자신에게 이벤트를 보냅니다. 상어의 어빌리티 중 이 태그를 'Trigger'로 설정한 어빌리티가 편지(Payload)를 받고 실행
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Shark, AttackAbilityTag, Payload);

	// 5. 작업 완료
	// 어빌리티의 실행 자체는 블루프린트에서 진행되므로, BT Task는 '명령 하달 완료'로 간주하고 즉시 성공을 반환
	// (공격 쿨타임은 BT의 Cooldown Decorator로 제어하는 것이 아키텍처 상 깔끔합니다.)
	//UE_LOG(LogTemp, Warning, TEXT("GAS Attack send Succeeded"));
	return EBTNodeResult::Succeeded;
}