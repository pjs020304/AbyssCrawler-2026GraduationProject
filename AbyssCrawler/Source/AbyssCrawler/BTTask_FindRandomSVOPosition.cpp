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
	// 매 프레임 상어를 경로를 따라 움직여야 하므로 Tick 활성화
	bNotifyTick = true;

	// 이 태스크는 AI마다 자기 경로와 스레드를 따로 들고 있어야 하므로 노드 인스턴싱을 켠다
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_FindRandomSVOPosition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Shark = OwnerComp.GetAIOwner()->GetPawn();
	ASVOVolume* SVOData = Cast<ASVOVolume>(UGameplayStatics::GetActorOfClass(GetWorld(), ASVOVolume::StaticClass()));

	if (!Shark || !SVOData) return EBTNodeResult::Failed;

	FVector Origin = Shark->GetActorLocation();
	FVector RandomPoint = FVector::ZeroVector;
	bool bFound = false;

	// 1. 주변 임의 방향으로 최대 10번 시도해, SVO 상에서 갈 수 있는 빈 공간(Walkable)을 찾는다
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

	// 찾지 못했다면 이번 순찰은 포기하고 실패로 돌려준다
	if (!bFound) return EBTNodeResult::Failed;

	// 찾은 좌표를 블랙보드에 기록한다 (디버그 및 다른 노드 참조용)
	OwnerComp.GetBlackboardComponent()->SetValueAsVector(RandomLocation.SelectedKeyName, RandomPoint);

	// ------------------------------------------------------------------
	// 2. 찾은 목적지까지의 A*를 워커 스레드에 맡긴다
	// ------------------------------------------------------------------
	FVector StartLoc = Shark->GetActorLocation();
	FVector TargetLoc = RandomPoint;

	bool bStartWalkable = SVOData->IsWalkable(StartLoc);
	if (!bStartWalkable)
	{
		StartLoc.Z += 300.0f; // 상어가 바닥에 파묻혀 있다면 위로 살짝 들어 올린다
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

	// --- [워커 스레드 결과 수신 파트] ---
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
			return; // 아직 계산 중이므로 이번 프레임은 움직이지 않고 대기
		}
	}

	// --- [경로를 따라가는 조향(Steering) 파트] ---
	if (CurrentPath.Num() > 0 && CurrentPath.IsValidIndex(CurrentWaypointIndex))
	{
		FVector TargetLocation = CurrentPath[CurrentWaypointIndex];
		FVector CurrentLocation = Shark->GetActorLocation();

		FVector DirectionToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();

		FRotator CurrentRotation = Shark->GetActorRotation();
		FRotator TargetRotation = DirectionToTarget.Rotation();

		FRotator SmoothedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, TurnSpeed);
		Shark->SetActorRotation(SmoothedRotation);

		Shark->AddMovementInput(Shark->GetActorForwardVector(), 1.0f); // 순찰 속도를 늦추려면 이 값을 0.5f 등으로 낮춘다

		float DistanceToWaypoint = FVector::Dist(CurrentLocation, TargetLocation);
		if (DistanceToWaypoint < AcceptanceRadius)
		{
			CurrentWaypointIndex++;

			if (CurrentWaypointIndex >= CurrentPath.Num())
			{
				// 마지막 경로점까지 도달했다면 순찰 성공으로 마무리
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