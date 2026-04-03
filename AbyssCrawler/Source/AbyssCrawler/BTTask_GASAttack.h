#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h" // GAS 태그를 사용하기 위해 필수 포함
#include "BTTask_GASAttack.generated.h"

UCLASS()
class ABYSSCRAWLER_API UBTTask_GASAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GASAttack();

protected:
	// Task가 실행될 때 호출되는 메인 함수
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 실행할 공격 어빌리티의 태그 ("Ability.Attack.Bite")
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	float AttackRange;
};