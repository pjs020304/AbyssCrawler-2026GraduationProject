#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
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

	// 찾아낸 랜덤 3D 좌표(FVector)를 저장할 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector RandomLocation;

	// 상어를 기준으로 어느 정도 반경 내에서 무작위 좌표를 찾을지 설정
	UPROPERTY(EditAnywhere, Category = "Search")
	float SearchRadius = 2000.0f;
};