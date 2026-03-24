#include "BTTask_SmoothChasePlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h" // 맵에 있는 액터를 찾기 위해 필요
#include "SVOPathfinder.h" // SVO 경로 탐색기 포함
#include "SVOVolume.h"              // ASVOVolume 클래스를 알기 위해 필요
#include "DrawDebugHelpers.h"// 디버그 드로잉을 위한 엔진 헤더

UBTTask_SmoothChasePlayer::UBTTask_SmoothChasePlayer()
{
	NodeName = TEXT("Smooth 3D Chase Async");

	// 매 프레임 상어를 움직여야 하므로 Tick 활성화
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_SmoothChasePlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ACharacter* Shark = Cast<ACharacter>(AIController->GetPawn());

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));

	if (!Shark || !TargetActor) return EBTNodeResult::Failed;

	// --- [SVO 경로 탐색 호출부] ---

	FVector StartLoc = Shark->GetActorLocation();
	FVector TargetLoc = TargetActor->GetActorLocation();

	ASVOVolume* SVOData = Cast<ASVOVolume>(UGameplayStatics::GetActorOfClass(GetWorld(), ASVOVolume::StaticClass()));

	if (!SVOData)
	{
		UE_LOG(LogTemp, Error, TEXT("SVOVolume Not Found !!"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Warning, TEXT("A* Finder Start"));

	// ------------------------------------------------------------------
	// [New] 원인 추적 진단 로직: 시작점과 도착점이 갈 수 있는 곳인지 확인
	// ------------------------------------------------------------------
	bool bStartWalkable = SVOData->IsWalkable(StartLoc);
	bool bTargetWalkable = SVOData->IsWalkable(TargetLoc);

	UE_LOG(LogTemp, Warning, TEXT("SVO Check -> Start Walkable: %d, Target Walkable: %d"), bStartWalkable, bTargetWalkable);

	// 1. 타겟(플레이어) 위치 안전 보정
	FVector SafeTargetLoc = TargetLoc;
	if (!bTargetWalkable)
	{
		SafeTargetLoc.Z += 300.0f;
		UE_LOG(LogTemp, Warning, TEXT("Target inside blocked voxel! Adjusted SafeTargetLoc Z +300."));
	}

	// 2. [New] 시작점(상어) 위치 안전 보정
	FVector SafeStartLoc = StartLoc;
	if (!bStartWalkable)
	{
		SafeStartLoc.Z += 300.0f; // 상어도 바닥에 파묻혀 있다면 위로 살짝 들어 올립니다.
		UE_LOG(LogTemp, Warning, TEXT("Shark inside blocked voxel! Adjusted SafeStartLoc Z +300."));
	}

	// 3. 인덱스 변환 시 Safe 위치들을 사용!
	float VoxelSize = 250.0f;
	FIntVector SafeStartIndex(
		FMath::RoundToInt(SafeStartLoc.X / VoxelSize),
		FMath::RoundToInt(SafeStartLoc.Y / VoxelSize),
		FMath::RoundToInt(SafeStartLoc.Z / VoxelSize)
	);

	FIntVector SafeTargetIndex(
		FMath::RoundToInt(SafeTargetLoc.X / VoxelSize),
		FMath::RoundToInt(SafeTargetLoc.Y / VoxelSize),
		FMath::RoundToInt(SafeTargetLoc.Z / VoxelSize)
	);

	// --- [멀티스레드 지시 파트] ---

	// 1. 기존에 돌고 있던 스레드가 있다면 안전하게 삭제 (메모리 누수 방지)
	if (PathfinderTask)
	{
		PathfinderTask->EnsureCompletion();
		delete PathfinderTask;
		PathfinderTask = nullptr;
	}

	// 2. 이전 경로 초기화
	CurrentPath.Empty();

	// 3. 백그라운드 스레드 생성 및 실행 (Fire and Forget)
	UE_LOG(LogTemp, Warning, TEXT("A* Thread Started..."));
	PathfinderTask = new FAsyncTask<FSVOPathfindingTask>(SVOData, SafeStartIndex, SafeStartLoc, SafeTargetIndex, SafeTargetLoc, VoxelSize);
	PathfinderTask->StartBackgroundTask();

	// 메인 스레드는 즉시 InProgress를 반환하여 프레임 드랍을 막습니다!
	return EBTNodeResult::InProgress;
}





void UBTTask_SmoothChasePlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ACharacter* Shark = Cast<ACharacter>(AIController->GetPawn());

	// --- [멀티스레드 결과 수신 파트] ---
	if (CurrentPath.Num() == 0 && PathfinderTask != nullptr)
	{
		// 스레드 연산이 끝났는지 물어봅니다.
		if (PathfinderTask->IsDone())
		{
			// 끝났다면 결과를 빼옵니다!
			CurrentPath = PathfinderTask->GetTask().ResultPath;

			// 다 쓴 스레드 청소
			delete PathfinderTask;
			PathfinderTask = nullptr;

			if (CurrentPath.Num() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("A* Thread Complete! NodeNum: %d"), CurrentPath.Num());
				CurrentWaypointIndex = 0;

				// [디버그 드로잉] 길찾기가 끝난 시점에 최종 선을 그어줍니다.
#if ENABLE_DRAW_DEBUG 
				UWorld* World = GetWorld();
				FVector DebugOffset = FVector(0.0f, 0.0f, 30.0f);
				for (int32 i = 0; i < CurrentPath.Num(); ++i) {
					FVector DrawLoc = CurrentPath[i] + DebugOffset;
					DrawDebugSphere(World, DrawLoc, 20.0f, 12, FColor::Green, false, 5.0f);
					if (i < CurrentPath.Num() - 1) {
						FVector NextDrawLoc = CurrentPath[i + 1] + DebugOffset;
						DrawDebugLine(World, DrawLoc, NextDrawLoc, FColor::Green, false, 5.0f, 0, 8.0f);
					}
				}
#endif
			}
			else {
				UE_LOG(LogTemp, Error, TEXT("A* Thread finished, but Path Not Found!"));
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}
		}
		else {
			// 아직 연산 중이라면 이번 프레임은 상어를 움직이지 않고 대기합니다.
			return;
		}
	}
	if (CurrentPath.Num() > 0 && CurrentPath.IsValidIndex(CurrentWaypointIndex)) {
		// 1. 현재 향해야 할 목적지 (웨이포인트)
		FVector TargetLocation = CurrentPath[CurrentWaypointIndex];
		FVector CurrentLocation = Shark->GetActorLocation();

		// 2. 조향 행동 (Steering Behavior) - 가고 싶은 방향 계산
		FVector DirectionToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();

		// 3. 부드러운 회전 (RInterpTo)
		// 상어가 팍! 하고 꺾이지 않고 서서히 목표 방향으로 머리를 돌립니다.
		FRotator CurrentRotation = Shark->GetActorRotation();
		FRotator TargetRotation = DirectionToTarget.Rotation();

		// Z축 회전(Pitch)도 포함하여 위아래로 자연스럽게 헤엄치게 합니다.
		FRotator SmoothedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, TurnSpeed);
		Shark->SetActorRotation(SmoothedRotation);

		// 4. 전진 추진력 부여 (AddMovementInput)
		// 머리가 향하고 있는 방향(ForwardVector)으로 지속적으로 헤엄칩니다.
		// 머리가 아직 덜 돌아갔어도 일단 앞으로 나아가기 때문에, 자연스럽게 크게 도는(Drifting) 궤적이 생깁니다.
		Shark->AddMovementInput(Shark->GetActorForwardVector(), 1.0f);

		// 5. 웨이포인트 도달 체크
		float DistanceToWaypoint = FVector::Dist(CurrentLocation, TargetLocation);
		if (DistanceToWaypoint < AcceptanceRadius)
		{
			// 목적지 반경에 들어왔다면 다음 경로점으로 인덱스를 넘깁니다.
			CurrentWaypointIndex++;

			// 만약 마지막 경로점까지 도달했다면 (플레이어 코앞까지 왔다면)
			if (CurrentWaypointIndex >= CurrentPath.Num())
			{
				// 추적 완료 판정 후 다음 Task(예: 물어뜯기 공격)로 넘어갑니다.
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
		}
	}
}