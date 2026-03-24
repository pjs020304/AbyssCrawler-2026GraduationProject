#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" // 감지 데이터 타입
#include "AbyssSharkAIController.generated.h"

// 전방 선언
class UBehaviorTree;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class ABYSSCRAWLER_API AAbyssSharkAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAbyssSharkAIController();

protected:
	// AI가 캐릭터에 빙의(Possess)할 때 실행 (Behavior Tree 시작점)
	virtual void OnPossess(APawn* InPawn) override;

	// --- [AI 핵심 컴포넌트] ---

	// 에디터에서 할당할 행동 트리(Behavior Tree)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* AIBehavior;

	// AI의 오감(Perception) 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* SharkPerceptionComp;

	// 시각(Sight) 설정 정보
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	// --- [감지 이벤트 함수] ---

	// 시야에 무언가 들어오거나 나갔을 때 실행될 함수
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);
};