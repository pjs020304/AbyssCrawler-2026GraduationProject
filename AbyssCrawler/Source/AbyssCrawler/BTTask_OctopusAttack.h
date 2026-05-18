#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_OctopusAttack.generated.h"

UCLASS()
class ABYSSCRAWLER_API UBTTask_OctopusAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_OctopusAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackRange;
};
