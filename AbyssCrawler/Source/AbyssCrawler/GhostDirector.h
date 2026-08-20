#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostDirector.generated.h"

class AGhostCreature;
class AAbyssDiverCharacter;

/**
 * 유령(원혼) 출현/디버프/포획을 총괄하는 서버 권위 디렉터.
 *
 * - 팀 전원이 수영 중일 때만 누적 타이머가 진행된다(누구든 수영이 아니면 0으로 리셋 + 전체 디스폰).
 * - 누적 300초에 1기, 이후 60초마다 1기 추가(최대 5기), 모든 플레이어로부터 일정 거리 밖에 스폰.
 * - 각 플레이어의 "유령 근접 누적 시간"으로 디버프 단계(1~3)를 결정하고, 범위 밖이면 회복.
 * - 포획(접촉) 시 해당 플레이어에게 즉사 처리를 요청한다.
 *
 * 레벨에 1개 배치하거나 BP로 만들어 배치한다.
 */
UCLASS()
class ABYSSCRAWLER_API AGhostDirector : public AActor
{
	GENERATED_BODY()

public:
	AGhostDirector();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- 스폰 ---

	// 스폰할 유령 클래스 (BP_GhostCreature 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost|Spawn")
	TSubclassOf<AGhostCreature> GhostClass;

	// 팀 전원 수영 누적 시간이 이 값을 넘으면 첫 유령 출현(초)
	UPROPERTY(EditAnywhere, Category = "Ghost|Spawn")
	float SpawnAfterSeconds = 300.f;

	// 이후 추가 유령 생성 간격(초)
	UPROPERTY(EditAnywhere, Category = "Ghost|Spawn")
	float SpawnInterval = 60.f;

	// 동시 최대 유령 수
	UPROPERTY(EditAnywhere, Category = "Ghost|Spawn")
	int32 MaxGhosts = 5;

	// 스폰 시 모든 플레이어로부터 최소 이 거리(uu) 밖에 생성 (100m = 10000)
	UPROPERTY(EditAnywhere, Category = "Ghost|Spawn")
	float SpawnMinPlayerDistance = 10000.f;

	// 스폰 위치 후보 탐색 시도 횟수
	UPROPERTY(EditAnywhere, Category = "Ghost|Spawn")
	int32 SpawnTryCount = 16;

	// --- 근접 디버프(Haunt) ---

	// 이 거리 안에 있으면 근접 시간이 누적된다.
	UPROPERTY(EditAnywhere, Category = "Ghost|Haunt")
	float HauntRadius = 800.f;

	// 단계별 누적 시간 임계(초): 1단계 둔화 / 2단계 시야왜곡 / 3단계 암전
	UPROPERTY(EditAnywhere, Category = "Ghost|Haunt")
	float Stage1Time = 3.f;
	UPROPERTY(EditAnywhere, Category = "Ghost|Haunt")
	float Stage2Time = 7.f;
	UPROPERTY(EditAnywhere, Category = "Ghost|Haunt")
	float Stage3Time = 12.f;

	// 범위 밖에서 근접 시간이 줄어드는 배율 (1 = 1초당 1초 회복)
	UPROPERTY(EditAnywhere, Category = "Ghost|Haunt")
	float RecoveryRate = 1.5f;

	// --- 포획(Catch) ---

	// 이 거리 안으로 들어오면 즉시 포획(즉사 진행).
	UPROPERTY(EditAnywhere, Category = "Ghost|Catch")
	float CatchRadius = 120.f;

private:
	float AllSwimmingTime = 0.f;
	TArray<TWeakObjectPtr<AGhostCreature>> ActiveGhosts;

	void GatherAliveDivers(TArray<AAbyssDiverCharacter*>& OutDivers) const;
	bool AreAllSwimming(const TArray<AAbyssDiverCharacter*>& Divers) const;
	void UpdateSpawning(const TArray<AAbyssDiverCharacter*>& Divers);
	void UpdateHauntAndCatch(float DeltaTime, const TArray<AAbyssDiverCharacter*>& Divers);
	bool FindSpawnLocation(const TArray<AAbyssDiverCharacter*>& Divers, FVector& OutLocation) const;
	int32 HauntStageFromTime(float TimeInRange) const;
	void DespawnAllGhosts();
	void PruneGhosts();
};
