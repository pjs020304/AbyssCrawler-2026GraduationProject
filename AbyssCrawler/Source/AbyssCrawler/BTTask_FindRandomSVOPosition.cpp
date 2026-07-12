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
	// �� ������ �� �������� �ϹǷ� Tick Ȱ��ȭ
	bNotifyTick = true;

	// �� �½�ũ�� AI(���)���� ���������� �����ϵ��� ����
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

	// 1. ������ �������� 10�� �˻��Ͽ� SVO �� �� �ִ� �� ����(Walkable) ã��
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

	// ã�� ���ߴٸ� ��� ����ϵ��� ���� ó��
	if (!bFound) return EBTNodeResult::Failed;

	// ã�� ��ǥ�� �������忡 ���� (����� �뵵)
	OwnerComp.GetBlackboardComponent()->SetValueAsVector(RandomLocation.SelectedKeyName, RandomPoint);

	// ------------------------------------------------------------------
	// 2. ã�� �������� A* ��Ƽ������ ��ã�� ����
	// ------------------------------------------------------------------
	FVector StartLoc = Shark->GetActorLocation();
	FVector TargetLoc = RandomPoint;

	bool bStartWalkable = SVOData->IsWalkable(StartLoc);
	if (!bStartWalkable)
	{
		StartLoc.Z += 300.0f; // �� �ٴڿ� �ִٸ� ����
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

	// --- [��Ƽ������ ��� ���� ��Ʈ] ---
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
			return; // ���� �� ���
		}
	}

	// --- [���� ���� �̵�(Steering) ��Ʈ] ---
	if (CurrentPath.Num() > 0 && CurrentPath.IsValidIndex(CurrentWaypointIndex))
	{
		FVector TargetLocation = CurrentPath[CurrentWaypointIndex];
		FVector CurrentLocation = Shark->GetActorLocation();

		FVector DirectionToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();

		FRotator CurrentRotation = Shark->GetActorRotation();
		FRotator TargetRotation = DirectionToTarget.Rotation();

		FRotator SmoothedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, TurnSpeed);
		Shark->SetActorRotation(SmoothedRotation);

		Shark->AddMovementInput(Shark->GetActorForwardVector(), 1.0f); // �ʿ� �� ���� �ӵ��� 0.5f ���� ����

		float DistanceToWaypoint = FVector::Dist(CurrentLocation, TargetLocation);
		if (DistanceToWaypoint < AcceptanceRadius)
		{
			CurrentWaypointIndex++;

			if (CurrentWaypointIndex >= CurrentPath.Num())
			{
				// ���� ������ ���� �� ���� ó��
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