#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GhostCreature.generated.h"

class AAbyssDiverCharacter;

/**
 * 바다의 원혼(유령) 크리쳐.
 *
 * - 모든 충돌을 무시(공격/벽 통과)하며 무적이다(데미지 처리/피격 콜리전 없음).
 * - 매 틱(서버) 가장 가까운 다이버를 향해 느리게 직선 이동한다(경로탐색 불필요).
 * - 근접 디버프/포획 판정은 AGhostDirector가 중앙에서 처리한다(이 클래스는 이동만).
 */
UCLASS()
class ABYSSCRAWLER_API AGhostCreature : public ACharacter
{
	GENERATED_BODY()

public:
	AGhostCreature();

	virtual void Tick(float DeltaTime) override;

	// 현재 추적 중인 타깃(가장 가까운 다이버)
	AAbyssDiverCharacter* GetCurrentTarget() const { return CurrentTarget.Get(); }

protected:
	virtual void BeginPlay() override;

	// 이동 속도(플레이어보다 느리게)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Move")
	float MoveSpeed = 150.f;

	// 진행 방향으로의 회전 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Move")
	float TurnSpeed = 2.f;

	// 타깃 재선정 주기(초)
	UPROPERTY(EditAnywhere, Category = "Ghost|Move")
	float RetargetInterval = 0.5f;

private:
	TWeakObjectPtr<AAbyssDiverCharacter> CurrentTarget;
	float RetargetAccum = 0.f;

	AAbyssDiverCharacter* FindNearestDiver() const;
};
