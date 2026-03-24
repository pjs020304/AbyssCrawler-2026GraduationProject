#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SVOChasePlayer.generated.h"

UCLASS()
class ABYSSCRAWLER_API UBTTask_SVOChasePlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SVOChasePlayer();

protected:
	// Task가 실행될 때 호출되는 메인 함수
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 매 프레임(Tick)마다 상어의 위치를 갱신하기 위해 필요
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 블랙보드에서 타겟(플레이어)을 가져오기 위한 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;
};