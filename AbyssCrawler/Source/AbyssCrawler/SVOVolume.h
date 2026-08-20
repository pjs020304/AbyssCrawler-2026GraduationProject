#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "SVOVolume.generated.h"

// 옥트리의 기본 단위 (노드)
struct FSVONode
{
	FVector Center;       // 노드의 중심 좌표
	float Extent;         // 노드의 반 크기 (Half-size)
	bool bIsBlocked;      // 장애물이 걸쳐 있는가?
	bool bIsLeaf;         // 더 이상 쪼개지 않는 말단(리프) 노드인가?


	// 자식 노드 8개 (자신이 Leaf가 아닐 때만 채워진다)
	TArray<TSharedPtr<FSVONode>> Children;

	FSVONode(FVector InCenter, float InExtent)
		: Center(InCenter), Extent(InExtent), bIsBlocked(false), bIsLeaf(true) {
	}
};

UCLASS()
class ABYSSCRAWLER_API ASVOVolume : public AActor
{
	GENERATED_BODY()

public:
	ASVOVolume();

protected:
	virtual void BeginPlay() override;

	// 옥트리가 스캔할 범위를 에디터에서 시각적으로 잡아 주는 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SVO")
	UBoxComponent* BoundsVolume;

	// 트리를 몇 단계까지 쪼갤지 결정 (클수록 정밀해지지만 메모리가 늘어난다)
	UPROPERTY(EditAnywhere, Category = "SVO")
	int32 MaxDepth = 5;

	// 스캔할 때 장애물로 세지 않을 액터들 (크리처, 플레이어 등)
	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;

private:
	// 트리의 최상위 뿌리 노드
	TSharedPtr<FSVONode> RootNode;

	// 1. 트리를 재귀적으로 구축하는 함수
	void BuildOctree(TSharedPtr<FSVONode> Node, int32 CurrentDepth);

	// [PCG 연동] 변경 영역과 겹치는 노드만 골라 재스캔하는 재귀 함수
	void RebuildNodeRecursive(TSharedPtr<FSVONode> Node, int32 CurrentDepth, const FBox& DirtyBounds);

	// 스캔 시 무시할 액터(캐릭터 등) 목록을 최신화
	void RefreshIgnoredActors();

	void DrawNodeDebugRecursive(TSharedPtr<FSVONode> Node) const;

public:
	// 2. 외부(Pathfinder)에서 특정 좌표가 비어 있는지 O(log N)으로 묻는 함수
	bool IsWalkable(const FVector& Location) const;

	// 3. 물리 질의 대신 SVO 데이터만으로 수행하는 3D 가시성 판정 (경로 평활화용)
	bool SVORaycast(const FVector& Start, const FVector& End) const;

	void DrawSVODebug() const;

	// [PCG 연동] 지오메트리가 바뀐 영역(DirtyBounds)만 부분 재빌드한다.
	// PCG 생성이 모두 끝난 뒤 매니저가 호출한다. 최초 빌드 전이면 전체 빌드로 폴백.
	void RebuildRegion(const FBox& DirtyBounds);

	// 블루프린트/디버그용: 중심 + 반경(Extent)으로 부분 재빌드
	UFUNCTION(BlueprintCallable, Category = "SVO")
	void RebuildRegionBox(FVector Center, FVector Extent);

public:
	// 블루프린트에서 키 입력으로 호출할 수 있게 노출
	UFUNCTION(BlueprintCallable, Category = "SVO Debug")
	void ToggleSVODebug();

private:
	bool bIsDebugVisible = false;
};