#include "BTTask_SVOChasePlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbyssSharkAIController.h"
#include "AbyssSharkCharacter.h"
#include "GameFramework/Actor.h"
// #include "SVOSubsystem.h" // (가정) SVO 매니저 헤더 파일

UBTTask_SVOChasePlayer::UBTTask_SVOChasePlayer()
{
	NodeName = TEXT("SVO Chase Player");
	bNotifyTick = true; // TickTask를 사용하기 위해 true로 설정
}

EBTNodeResult::Type UBTTask_SVOChasePlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 1. 컨트롤러와 폰(상어) 가져오기
	AAbyssSharkAIController* AIController = Cast<AAbyssSharkAIController>(OwnerComp.GetAIOwner());
	if (!AIController) return EBTNodeResult::Failed;

	AAbyssSharkCharacter* Shark = Cast<AAbyssSharkCharacter>(AIController->GetPawn());
	if (!Shark) return EBTNodeResult::Failed;

	// 2. 블랙보드에서 타겟(플레이어) 위치 가져오기
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!TargetActor) return EBTNodeResult::Failed;

	// 3. SVO 매니저를 통해 A* 경로 도출 (앞서 논의한 String Pulling 포함)
	// USVOSubsystem* SVOManager = GetWorld()->GetSubsystem<USVOSubsystem>();
	// TArray<FVector> PathPoints = SVOManager->FindSmoothedPath(Shark->GetActorLocation(), TargetActor->GetActorLocation());

	// (임시) 경로가 있다고 가정
	bool bFoundPath = true;

	if (bFoundPath)
	{
		// 경로를 찾았다면, 상어의 메모리(또는 AIController)에 경로 배열을 저장합니다.
		// AIController->SetCurrentPath(PathPoints);

		// 즉시 종료하지 않고, 상어가 목표에 도달할 때까지 Task를 '진행 중' 상태로 유지합니다.
		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}

void UBTTask_SVOChasePlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 이 함수는 ExecuteTask가 'InProgress'를 반환했을 때 매 프레임 실행됩니다.

	AAbyssSharkAIController* AIController = Cast<AAbyssSharkAIController>(OwnerComp.GetAIOwner());
	AAbyssSharkCharacter* Shark = Cast<AAbyssSharkCharacter>(AIController->GetPawn());

	// 1. 저장해둔 경로(PathPoints)에서 현재 향해야 할 '다음 노드'를 꺼냅니다.
	FVector NextWaypoint = /* AIController->GetNextWaypoint() */ FVector::ZeroVector;

	// 2. 스티어링(Steering Behavior) 적용: 상어의 머리를 돌리고 앞으로 헤엄치게 합니다.
	FVector Direction = (NextWaypoint - Shark->GetActorLocation()).GetSafeNormal();

	// 머리 회전 (부드럽게)
	FRotator TargetRot = Direction.Rotation();
	Shark->SetActorRotation(FMath::RInterpTo(Shark->GetActorRotation(), TargetRot, DeltaSeconds, 5.0f));

	// 전진 추진력 (AddMovementInput)
	Shark->AddMovementInput(Shark->GetActorForwardVector(), 1.0f);

	// 3. 다음 웨이포인트 반경 안에 도착했다면?
	if (FVector::Distance(Shark->GetActorLocation(), NextWaypoint) < 150.0f)
	{
		// 경로 배열의 인덱스를 다음으로 넘깁니다.
		// 만약 경로의 마지막(플레이어 코앞)에 도달했다면 Task를 성공으로 끝냅니다.
		// FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}