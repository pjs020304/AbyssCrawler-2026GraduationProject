#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Engine/TimerHandle.h"
#include "Perception/AIPerceptionTypes.h" // 감지 자극(FAIStimulus) 타입
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
	// AI가 캐릭터에 빙의(Possess)할 때 호출된다 (여기서 Behavior Tree를 실행한다)
	virtual void OnPossess(APawn* InPawn) override;

	// --- [AI 핵심 컴포넌트] ---

	// 에디터에서 지정하는 행동 트리(Behavior Tree)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* AIBehavior;

	// AI 감지(Perception) 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* SharkPerceptionComp;

	// 시각(Sight) 감지 설정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	// --- [감지 이벤트 함수] ---

	// 시야에 대상이 들어오거나 벗어났을 때 호출되는 함수
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

	// --- [추적 해제: 1단계 흥미 상실 + 3단계 영역 리쉬] ---

	// 시야를 놓친 뒤 마지막 목격 위치를 수색하다가 추적을 포기하기까지의 시간(초).
	UPROPERTY(EditDefaultsOnly, Category = "AI|Chase")
	float LoseInterestTime = 8.0f;

	// 둥지(스폰 위치)에서 이 거리 이상 벗어나면 추적을 포기하고 복귀.
	UPROPERTY(EditDefaultsOnly, Category = "AI|Chase")
	float HomeLeashRadius = 6000.0f;

	// 영역 리쉬 검사 주기(초).
	UPROPERTY(EditDefaultsOnly, Category = "AI|Chase")
	float LeashCheckInterval = 0.5f;

	// 블랙보드 키 이름 (Behavior Tree의 키와 반드시 일치해야 함).
	UPROPERTY(EditDefaultsOnly, Category = "AI|Chase")
	FName TargetActorKey = TEXT("TargetActor");
	UPROPERTY(EditDefaultsOnly, Category = "AI|Chase")
	FName HasLineOfSightKey = TEXT("HasLineOfSight");
	UPROPERTY(EditDefaultsOnly, Category = "AI|Chase")
	FName LastKnownLocationKey = TEXT("LastKnownLocation");
	UPROPERTY(EditDefaultsOnly, Category = "AI|Chase")
	FName HomeLocationKey = TEXT("HomeLocation");

	// 스폰(둥지) 위치. OnPossess에서 기록.
	FVector HomeLocation = FVector::ZeroVector;

	FTimerHandle GiveUpTimerHandle;
	FTimerHandle LeashCheckTimerHandle;

	// 흥미 상실 타이머 만료 → 추적 포기.
	void OnGiveUpChase();

	// 주기적으로 둥지와의 거리를 검사(3단계 영역 리쉬).
	void CheckHomeLeash();

	// 타겟을 비우고 추적을 종료(타이머 정리 포함). BT는 이후 순찰/복귀로 폴백.
	void AbandonChase();
};