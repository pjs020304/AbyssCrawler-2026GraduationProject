// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssPCGManager.generated.h"

class ASVOVolume;
class UPCGComponent;
class AAbyssItemBase;

// PCG 아이템 배치용 가중치 스폰 테이블 엔트리
USTRUCT()
struct FAbyssPCGItemEntry
{
	GENERATED_BODY()

	// 스폰할 아이템 클래스 (리플리케이트되는 AAbyssItemBase 계열)
	UPROPERTY(EditAnywhere)
	TSubclassOf<AAbyssItemBase> ItemClass;

	// 선택 가중치 (높을수록 자주 스폰)
	UPROPERTY(EditAnywhere)
	float Weight = 1.0f;
};

/**
 * 심해 환경 PCG(산호/암초/아이템 등)를 조율하는 매니저.
 *
 * 핵심 책임 2가지:
 *  1) [결정론 동기화] 서버가 랜덤 시드를 정해 리플리케이트 → 모든 클라이언트가
 *     동일 시드로 PCG를 생성하므로 화면상 배치가 완전히 동일해진다.
 *  2) [연산 순서 보장] "PCG 생성 완료" → "변경 영역(DirtyBounds) 수집" → "SVO 부분 재빌드"
 *     순서를 델리게이트 기반으로 강제한다. 모든 PCG 컴포넌트가 끝나기 전에는 SVO를 건드리지 않는다.
 *
 * 배치 방법:
 *  - 레벨에 이 액터 1개를 배치한다.
 *  - PCGActors 에 PCG 컴포넌트를 가진 액터(들)을 지정한다. (비우면 레벨 전체에서 자동 수집)
 *  - TargetSVOVolume 에 SVO 볼륨을 지정한다. (비우면 자동 탐색)
 *  - 각 PCG 컴포넌트의 GenerationTrigger 는 GenerateOnDemand 로 두는 것을 권장.
 */
UCLASS()
class ABYSSCRAWLER_API AAbyssPCGManager : public AActor
{
	GENERATED_BODY()

public:
	AAbyssPCGManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// 이 매니저가 제어할 PCG 액터. 비워두면 BeginPlay에서 레벨 전체를 자동 수집한다.
	UPROPERTY(EditAnywhere, Category = "Abyss PCG")
	TArray<TObjectPtr<AActor>> PCGActors;

	// 재빌드 대상 SVO 볼륨. 비워두면 자동 탐색.
	UPROPERTY(EditAnywhere, Category = "Abyss PCG")
	TObjectPtr<ASVOVolume> TargetSVOVolume;

	// 변경 영역을 SVO에 넘길 때 여유 마진(cm). 콜리전 경계 근처 보정용.
	UPROPERTY(EditAnywhere, Category = "Abyss PCG")
	float DirtyBoundsPadding = 200.0f;

	// ── 아이템 배치 (서버 전용 스폰) ─────────────────────────────
	// 아이템 후보 위치를 뽑는 PCG 액터. 장식 PCGActors와 반드시 분리할 것.
	// 서버에서만 Generate되고, 그래프는 포인트를 Output 노드에 직결해야 한다 (스포너 노드 금지).
	UPROPERTY(EditAnywhere, Category = "Abyss PCG|Items")
	TArray<TObjectPtr<AActor>> ItemPCGActors;

	// 가중치 스폰 테이블 (예: 산소통 3.0 / 배터리 2.0 / 희귀광물 0.5)
	UPROPERTY(EditAnywhere, Category = "Abyss PCG|Items")
	TArray<FAbyssPCGItemEntry> ItemSpawnTable;

	// PCG가 뽑은 후보 포인트 중 실제로 스폰할 최대 개수 (아이템 PCG 컴포넌트당)
	UPROPERTY(EditAnywhere, Category = "Abyss PCG|Items")
	int32 MaxItemSpawnCount = 20;

	// 스폰 위치 Z 보정(cm). 바닥 파묻힘 방지
	UPROPERTY(EditAnywhere, Category = "Abyss PCG|Items")
	float ItemSpawnZOffset = 30.0f;

	// 서버가 정해서 리플리케이트하는 시드. OnRep에서 클라 생성이 시작된다.
	UPROPERTY(ReplicatedUsing = OnRep_Seed)
	int32 ReplicatedSeed = 0;

	UFUNCTION()
	void OnRep_Seed();

private:
	// 시드 적용 → 전 PCG 생성 트리거 → 완료 델리게이트 바인딩
	void StartGeneration(int32 InSeed);

	// 개별 PCG 컴포넌트 생성 완료 콜백 (모두 끝나면 SVO 재빌드)
	void HandlePCGGenerated(UPCGComponent* InComponent);

	// PCGActors(또는 레벨 전체)에서 UPCGComponent들을 수집 (ItemPCGActors는 제외)
	TArray<UPCGComponent*> CollectPCGComponents() const;

	// ── 아이템 배치 경로 (서버 전용) ──
	// 아이템 PCG 생성 트리거
	void StartItemGeneration();

	// 개별 아이템 PCG 완료 콜백: Output 포인트를 읽어 리플리케이트 아이템 스폰
	void HandleItemPCGGenerated(UPCGComponent* InComponent);

	// ItemPCGActors에서 UPCGComponent들을 수집
	TArray<UPCGComponent*> CollectItemPCGComponents() const;

	// 스폰 테이블에서 가중치 합 비례로 아이템 클래스 선택
	TSubclassOf<AAbyssItemBase> PickWeightedItemClass(FRandomStream& Rng) const;

	// 아직 생성이 끝나지 않은 PCG 컴포넌트 수
	int32 PendingComponents = 0;

	// 모든 PCG가 변경한 영역의 합집합 (SVO에 전달)
	FBox AccumulatedDirtyBounds;

	// 중복 실행 방지
	bool bGenerationStarted = false;
};
