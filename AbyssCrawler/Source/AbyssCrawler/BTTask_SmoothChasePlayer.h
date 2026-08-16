#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Async/AsyncWork.h"
#include "SVOPathfinder.h"

#include "BTTask_SmoothChasePlayer.generated.h"

class FSVOPathfindingTask;

UCLASS()
class ABYSSCRAWLER_API UBTTask_SmoothChasePlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SmoothChasePlayer();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 추적이 중단(어보트)되면 돌고 있는 A* 스레드를 정리해야 한다.
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 성공/실패/어보트 어느 경로로 끝나도 스레드를 반납하는 마지막 관문.
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// 쫓아갈 대상(플레이어)을 가져올 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	// 웨이포인트(경로점)에 이만큼 가까워지면 다음 점으로 넘어감
	UPROPERTY(EditAnywhere, Category = "Steering")
	float AcceptanceRadius = 400.0f;

	// 상어가 몸을 돌리는 회전 속도 (낮을수록 크게 돎)
	UPROPERTY(EditAnywhere, Category = "Steering")
	float TurnSpeed = 3.0f;

private:
	// 돌고 있는 A* 스레드를 안전하게 취소하고 반납한다.
	// 취소가 불가능하면(이미 워커가 집어간 경우) 끝날 때까지만 기다리고,
	// 아직 시작도 안 했다면 게임 스레드에서 대신 계산하지 않고 버린다.
	void ReleasePathfinderTask();

	// A* 알고리즘이 뱉어낸 경로점들을 담아둘 배열
	TArray<FVector> CurrentPath;

	// 현재 상어가 향하고 있는 경로점의 인덱스
	int32 CurrentWaypointIndex = 0;

	// 백그라운드 작업을 관리할 포인터
	FAsyncTask<FSVOPathfindingTask>* PathfinderTask = nullptr;
};

// ------------------------------------------------------------------------------------------------------------
// ------------------------------------ PathFinding Task Thread Class -----------------------------------------
// ------------------------------------------------------------------------------------------------------------

// 백그라운드에서 A* 알고리즘을 수행할 전용 Task 클래스
class FSVOPathfindingTask : public FNonAbandonableTask
{
public:
	// 1. 입력받을 인자들 (Inputs)
	ASVOVolume* SVOData;
	FIntVector StartIndex;
	FVector StartLoc;
	FIntVector TargetIndex;
	FVector TargetLoc;
	float VoxelSize;

	// 2. 메인 스레드로 돌려줄 결과물 (Output)
	TArray<FVector> ResultPath;

	// 3. 생성자: 메인 스레드에서 스레드를 생성할 때 인자 값을 받아옵니다.
	FSVOPathfindingTask(ASVOVolume* InSVOData, FIntVector InStartIndex, FVector InStartLoc, FIntVector InTargetIndex, FVector InTargetLoc, float InVoxelSize)
		: SVOData(InSVOData), StartIndex(InStartIndex), StartLoc(InStartLoc), TargetIndex(InTargetIndex), TargetLoc(InTargetLoc), VoxelSize(InVoxelSize)
	{
	}

	// 4. 언리얼 프로파일러(Stat) 추적을 위한 매크로 (필수 보일러플레이트 코드)
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FSVOPathfindingTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	// 5. [가장 중요] 백그라운드 스레드에서 실제 실행될 함수!
	void DoWork()
	{
		// 이 안의 코드는 메인 스레드와 완전히 분리되어 실행되므로 게임 프레임에 영향을 주지 않습니다.

		// 무거운 A* 연산을 돌립니다.
		TArray<FVector> RawPath = FSVOPathfinder::FindPath(SVOData, StartIndex, StartLoc, TargetIndex, TargetLoc, VoxelSize);

		// 스트링 풀링도 연산이 무거우니 아예 백그라운드에서 같이 해버립니다!
		if (RawPath.Num() > 0)
		{
			ResultPath = FSVOPathfinder::SmoothPath(SVOData, RawPath);
		}
	}
};