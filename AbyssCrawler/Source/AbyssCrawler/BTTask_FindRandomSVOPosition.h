#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SmoothChasePlayer.h"
#include "BTTask_FindRandomSVOPosition.generated.h"

UCLASS()
class ABYSSCRAWLER_API UBTTask_FindRandomSVOPosition : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindRandomSVOPosition();

protected:
	// Task가 실행될 때 호출되는 메인 함수
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// 찾아낸 랜덤 3D 좌표(FVector)를 저장할 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector RandomLocation;

	// 상어를 기준으로 어느 정도 반경 내에서 무작위 좌표를 찾을지 설정
	UPROPERTY(EditAnywhere, Category = "Search")
	float SearchRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float AcceptanceRadius = 400.0f;

	// 정찰 시에는 추적(3.0f)보다 부드럽고 천천히 돌도록 기본값을 낮춥니다.
	UPROPERTY(EditAnywhere, Category = "Steering")
	float TurnSpeed = 1.5f;

private:
	TArray<FVector> CurrentPath;
	int32 CurrentWaypointIndex;
	FAsyncTask<FSVOPathfindingTask>* PathfinderTask = nullptr;
};
