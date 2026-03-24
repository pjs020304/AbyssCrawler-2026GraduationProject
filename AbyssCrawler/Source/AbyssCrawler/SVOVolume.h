#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "SVOVolume.generated.h"

// 옥트리의 기본 단위 (노드)
struct FSVONode
{
	FVector Center;       // 복셀의 중심 좌표
	float Extent;         // 복셀의 절반 크기 (Half-size)
	bool bIsBlocked;      // 장애물이 있는가?
	bool bIsLeaf;         // 더 이상 쪼개지지 않는 끝단(말단) 노드인가?


	// 자식 노드 8개 (자신이 Leaf가 아닐 때만 생성됨)
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

	// 맵 스캔 범위를 시각적으로 보여줄 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SVO")
	UBoxComponent* BoundsVolume;

	// 트리가 몇 번이나 쪼개질지 결정 (값이 클수록 정밀하지만 메모리 증가)
	UPROPERTY(EditAnywhere, Category = "SVO")
	int32 MaxDepth = 5;

	// 스캔 시 무시할 캐릭터들 (상어, 플레이어 등)
	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;

private:
	// 트리의 최상단 뿌리 노드
	TSharedPtr<FSVONode> RootNode;

	// 1. 트리를 재귀적으로 생성하는 함수
	void BuildOctree(TSharedPtr<FSVONode> Node, int32 CurrentDepth);

	void DrawNodeDebugRecursive(TSharedPtr<FSVONode> Node) const;

public:
	// 2. 외부(Pathfinder)에서 특정 좌표가 막혀있는지 $O(\log N)$ 속도로 물어볼 함수
	bool IsWalkable(const FVector& Location) const;

	// 3. 물리 엔진 대신 SVO 데이터를 활용한 초고속 3D 광선 추적 (스트링 풀링용)
	bool SVORaycast(const FVector& Start, const FVector& End) const;

	void DrawSVODebug() const;

public:
	// 블루프린트에서 키 입력으로 호출할 수 있게 노출
	UFUNCTION(BlueprintCallable, Category = "SVO Debug")
	void ToggleSVODebug();

private:
	bool bIsDebugVisible = false;
};