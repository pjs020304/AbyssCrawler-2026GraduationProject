#include "BTTask_FindRandomSVOPosition.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "SVOVolume.h"
#include "SVOPathfinder.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_FindRandomSVOPosition::UBTTask_FindRandomSVOPosition()
{
	NodeName = TEXT("SVO Patrol & Move");
	// 매 프레임 상어를 움직여야 하므로 Tick 활성화
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_FindRandomSVOPosition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Shark = OwnerComp.GetAIOwner()->GetPawn();
	ASVOVolume* SVOData = Cast<ASVOVolume>(UGameplayStatics::GetActorOfClass(GetWorld(), ASVOVolume::StaticClass()));

	if (!Shark || !SVOData) return EBTNodeResult::Failed;

	FVector Origin = Shark->GetActorLocation();
	FVector RandomPoint;
	bool bFound = false;

	// 1. 랜덤한 방향으로 10번 검사하여 SVO 갈 수 있는 빈 공간(Walkable) 찾기
	for (int i = 0; i < 10; ++i)
	{
		FVector RandomDir = FMath::VRand();
		FVector CandidateLoc = Origin + (RandomDir * FMath::RandRange(1000.0f, 3000.0f));

		if (SVOData->IsWalkable(CandidateLoc))
		{
			RandomPoint = CandidateLoc;
			bFound = true;
			break;
		}
	}

	// 찾지 못했다면 잠시 대기하도록 실패 처리
	if (!bFound) return EBTNodeResult::Failed;

	// 찾은 좌표를 블랙보드에 저장 (디버깅 용도)
	OwnerComp.GetBlackboardComponent()->SetValueAsVector(RandomLocation.SelectedKeyName, RandomPoint);

	// ------------------------------------------------------------------
	// 2. 찾은 목적지로 A* 멀티스레드 길찾기 시작
	// ------------------------------------------------------------------
	FVector StartLoc = Shark->GetActorLocation();
	FVector TargetLoc = RandomPoint;

	bool bStartWalkable = SVOData->IsWalkable(StartLoc);
	if (!bStartWalkable)
	{
		StartLoc.Z += 300.0f; // 상어가 바닥에 있다면 보정
	}

	float VoxelSize = 250.0f;
	FIntVector SafeStartIndex(
		FMath::RoundToInt(StartLoc.X / VoxelSize),
		FMath::RoundToInt(StartLoc.Y / VoxelSize),
		FMath::RoundToInt(StartLoc.Z / VoxelSize)
	);

	FIntVector SafeTargetIndex(
		FMath::RoundToInt(TargetLoc.X / VoxelSize),
		FMath::RoundToInt(TargetLoc.Y / VoxelSize),
		FMath::RoundToInt(TargetLoc.Z / VoxelSize)
	);

	if (PathfinderTask)
	{
		PathfinderTask->EnsureCompletion();
		delete PathfinderTask;
		PathfinderTask = nullptr;
	}

	CurrentPath.Empty();

	//UE_LOG(LogTemp, Warning, TEXT("Patrol A* Thread Started..."));
	PathfinderTask = new FAsyncTask<FSVOPathfindingTask>(SVOData, SafeStartIndex, StartLoc, SafeTargetIndex, TargetLoc, VoxelSize);
	PathfinderTask->StartBackgroundTask();

	return EBTNodeResult::InProgress;
}

void UBTTask_FindRandomSVOPosition::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ACharacter* Shark = Cast<ACharacter>(AIController->GetPawn());

	if (!Shark)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// --- [멀티스레드 결과 수신 파트] ---
	if (CurrentPath.Num() == 0 && PathfinderTask != nullptr)
	{
		if (PathfinderTask->IsDone())
		{
			CurrentPath = PathfinderTask->GetTask().ResultPath;

			delete PathfinderTask;
			PathfinderTask = nullptr;

			if (CurrentPath.Num() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Patrol A* Complete!"));
				CurrentWaypointIndex = 0;
			}
			else
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}
		}
		else
		{
			return; // 연산 중 대기
		}
	}

	// --- [실제 정찰 이동(Steering) 파트] ---
	if (CurrentPath.Num() > 0 && CurrentPath.IsValidIndex(CurrentWaypointIndex))
	{
		FVector TargetLocation = CurrentPath[CurrentWaypointIndex];
		FVector CurrentLocation = Shark->GetActorLocation();

		FVector DirectionToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();

		FRotator CurrentRotation = Shark->GetActorRotation();
		FRotator TargetRotation = DirectionToTarget.Rotation();

		FRotator SmoothedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, TurnSpeed);
		Shark->SetActorRotation(SmoothedRotation);

		Shark->AddMovementInput(Shark->GetActorForwardVector(), 1.0f); // 필요 시 정찰 속도로 0.5f 설정 가능

		float DistanceToWaypoint = FVector::Dist(CurrentLocation, TargetLocation);
		if (DistanceToWaypoint < AcceptanceRadius)
		{
			CurrentWaypointIndex++;

			if (CurrentWaypointIndex >= CurrentPath.Num())
			{
				// 최종 목적지 도달 시 성공 처리
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
		}
	}
}

void UBTTask_FindRandomSVOPosition::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	if (PathfinderTask)
	{
		PathfinderTask->EnsureCompletion();
		delete PathfinderTask;
		PathfinderTask = nullptr;
	}
}