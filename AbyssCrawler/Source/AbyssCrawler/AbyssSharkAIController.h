#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Engine/TimerHandle.h"
#include "Perception/AIPerceptionTypes.h" // ���� ������ Ÿ��
#include "AbyssSharkAIController.generated.h"

// ���� ����
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
	// AI�� ĳ���Ϳ� ����(Possess)�� �� ���� (Behavior Tree ������)
	virtual void OnPossess(APawn* InPawn) override;

	// --- [AI �ٽ� ������Ʈ] ---

	// �����Ϳ��� �Ҵ��� �ൿ Ʈ��(Behavior Tree)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* AIBehavior;

	// AI�� ����(Perception) ������Ʈ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* SharkPerceptionComp;

	// �ð�(Sight) ���� ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	// --- [���� �̺�Ʈ �Լ�] ---

	// �þ߿� ���� �����ų� ������ �� ����� �Լ�
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