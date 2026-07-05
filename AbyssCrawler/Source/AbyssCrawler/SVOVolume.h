#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "SVOVolume.generated.h"

// ��Ʈ���� �⺻ ���� (���)
struct FSVONode
{
	FVector Center;       // ������ �߽� ��ǥ
	float Extent;         // ������ ���� ũ�� (Half-size)
	bool bIsBlocked;      // ��ֹ��� �ִ°�?
	bool bIsLeaf;         // �� �̻� �ɰ����� �ʴ� ����(����) ����ΰ�?


	// �ڽ� ��� 8�� (�ڽ��� Leaf�� �ƴ� ���� ������)
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

	// �� ��ĵ ������ �ð������� ������ �ڽ�
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SVO")
	UBoxComponent* BoundsVolume;

	// Ʈ���� �� ���̳� �ɰ����� ���� (���� Ŭ���� ���������� �޸� ����)
	UPROPERTY(EditAnywhere, Category = "SVO")
	int32 MaxDepth = 5;

	// ��ĵ �� ������ ĳ���͵� (���, �÷��̾� ��)
	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;

private:
	// Ʈ���� �ֻ�� �Ѹ� ���
	TSharedPtr<FSVONode> RootNode;

	// 1. Ʈ���� ��������� �����ϴ� �Լ�
	void BuildOctree(TSharedPtr<FSVONode> Node, int32 CurrentDepth);

	// [PCG 연동] 변경 영역과 겹치는 노드만 골라 재스캔하는 재귀 함수
	void RebuildNodeRecursive(TSharedPtr<FSVONode> Node, int32 CurrentDepth, const FBox& DirtyBounds);

	// 스캔 시 무시할 액터(캐릭터 등) 목록을 최신화
	void RefreshIgnoredActors();

	void DrawNodeDebugRecursive(TSharedPtr<FSVONode> Node) const;

public:
	// 2. �ܺ�(Pathfinder)���� Ư�� ��ǥ�� �����ִ��� $O(\log N)$ �ӵ��� ��� �Լ�
	bool IsWalkable(const FVector& Location) const;

	// 3. ���� ���� ��� SVO �����͸� Ȱ���� �ʰ��� 3D ���� ���� (��Ʈ�� Ǯ����)
	bool SVORaycast(const FVector& Start, const FVector& End) const;

	void DrawSVODebug() const;

	// [PCG 연동] 지오메트리가 바뀐 영역(DirtyBounds)만 부분 재빌드한다.
	// PCG 생성이 모두 끝난 뒤 매니저가 호출한다. 최초 빌드 전이면 전체 빌드로 폴백.
	void RebuildRegion(const FBox& DirtyBounds);

	// 블루프린트/디버그용: 중심 + 반경(Extent)으로 부분 재빌드
	UFUNCTION(BlueprintCallable, Category = "SVO")
	void RebuildRegionBox(FVector Center, FVector Extent);

public:
	// ��������Ʈ���� Ű �Է����� ȣ���� �� �ְ� ����
	UFUNCTION(BlueprintCallable, Category = "SVO Debug")
	void ToggleSVODebug();

private:
	bool bIsDebugVisible = false;
};