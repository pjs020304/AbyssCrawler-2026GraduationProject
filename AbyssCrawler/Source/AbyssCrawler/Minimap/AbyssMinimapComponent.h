#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Minimap/AbyssMinimapTypes.h"
#include "AbyssMinimapComponent.generated.h"

class AAbyssSubmarine;

/**
 * 미니맵에 표시할 위치 스냅샷을 서버에서 모아 전 클라이언트로 복제하는 컴포넌트.
 *
 * AAbyssGameState에 부착된다. GameState는 bAlwaysRelevant(GameStateBase.cpp)이므로
 * 이 컴포넌트의 복제 프로퍼티도 거리와 무관하게 모든 클라이언트에 도달한다.
 * 덕분에 원거리 캐릭터/미션 액터의 릴리번시를 건드리지 않고도 (= 무브먼트/애니메이션
 * 풀 복제 비용 없이) 미니맵이 맵 전체를 보여줄 수 있다.
 */
UCLASS()
class ABYSSCRAWLER_API UAbyssMinimapComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbyssMinimapComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 미션 구성이 바뀌었음을 서버에 알린다. 여러 번 불려도 다음 틱에 한 번만 재구축된다.
	void RequestStaticRebuild();

	// 매 갱신마다 움직이는 대상: 플레이어 + 잠수함
	const TArray<FAbyssMinimapEntry>& GetDynamicEntries() const { return DynamicEntries; }

	// 미션 수락/완료 시에만 바뀌는 대상: 활성 미션 목표 액터
	const TArray<FAbyssMinimapEntry>& GetStaticEntries() const { return StaticEntries; }

protected:
	UPROPERTY(Replicated)
	TArray<FAbyssMinimapEntry> DynamicEntries;

	UPROPERTY(Replicated)
	TArray<FAbyssMinimapEntry> StaticEntries;

	// 10Hz. AActor 기본 NetUpdateFrequency가 100Hz이므로 이 타이머가 실질 전송 주기가 된다.
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float DynamicUpdateInterval = 0.1f;

private:
	void UpdateDynamicEntries();
	void RebuildStaticEntries();

	FTimerHandle DynamicUpdateTimer;
	bool bStaticRebuildPending = false;
	TWeakObjectPtr<AAbyssSubmarine> CachedSubmarine;
};
