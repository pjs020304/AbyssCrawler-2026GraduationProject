// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssReviveStation.generated.h"

class UWidgetComponent;
class AAbyssPlayerState;

/**
 * 체력 스테이션 (부활 + 체력 충전).
 * E키 상호작용 시
 *   - 사용한 플레이어가 살아 있으면 체력을 최대치로 충전하고,
 *   - 죽어 있는 모든 플레이어를 PlayerStart 지점에 되살린다.
 * 둘 중 하나라도 성사되면 쿨타임에 들어간다.
 *
 * 산소/배터리 스테이션(AAbyssRechargeStation)과 달리 체력 충전은 GE 에셋이 아니라
 * 어트리뷰트를 직접 세팅한다. GE_RestoreHealthMax에 해당하는 에셋이 없기 때문.
 *
 * 부활 파이프라인 (전부 서버에서 실행):
 *  ① 죽은 캐릭터 껍데기 정리: 인벤토리 드롭 → 액터 파괴
 *  ② bIsAlive 복구 → 관전 상태 해제 → GameMode::RestartPlayer로 PlayerStart에 재스폰+빙의
 *  ③ 새로 스폰된 캐릭터의 체력/산소를 최대치로 보정
 *  ④ 부활한 플레이어의 시체(CorpseItem) 제거 (운반 중인 시체는 제외)
 */
UCLASS()
class ABYSSCRAWLER_API AAbyssReviveStation : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()

public:
	AAbyssReviveStation();

	// E키 상호작용
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

protected:
	// [서버] 살아 있는 사용자의 체력을 최대치로 충전. 실제로 회복했으면 true
	bool RestoreHealth(class AAbyssDiverCharacter* Diver);

	// [서버] 한 명 부활 처리. 성공 시 true
	bool RevivePlayer(AAbyssPlayerState* DeadPlayerState);

	// [서버] 죽은 캐릭터 껍데기 정리 (아이템 드롭 후 파괴)
	void CleanupDeadCharacters();

	// [서버] 부활한 플레이어의 시체 아이템 제거
	void RemoveCorpseOf(const AAbyssPlayerState* PlayerState);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	// "E키로 부활" 안내 UI
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UWidgetComponent* InteractWidget;

	// 사용 후 재사용 대기시간(초). 연타 방지
	UPROPERTY(EditDefaultsOnly, Category = "Station Config")
	float ReviveCooldown = 3.0f;

	// 부활 1인당 소모할 팀 공유 재화. 0이면 무료
	UPROPERTY(EditDefaultsOnly, Category = "Station Config")
	int32 ReviveCostPerPlayer = 0;

private:
	bool bIsOnCooldown = false;
	FTimerHandle CooldownTimerHandle;
	void ResetCooldown();
};
