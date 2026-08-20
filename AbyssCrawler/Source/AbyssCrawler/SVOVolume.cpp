#include "SVOVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

ASVOVolume::ASVOVolume()
{
	// 이 액터는 매 프레임 할 일이 없다. Tick을 끈다.
	PrimaryActorTick.bCanEverTick = false;

	BoundsVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsVolume"));
	RootComponent = BoundsVolume;
	BoundsVolume->SetBoxExtent(FVector(5000.0f, 5000.0f, 5000.0f));
}

void ASVOVolume::BeginPlay()
{
	Super::BeginPlay();

	// 맵에 있는 모든 캐릭터(크리처, 플레이어 등)를 찾아 무시 목록에 넣는다.
	RefreshIgnoredActors();

	RootNode = MakeShared<FSVONode>(GetActorLocation(), BoundsVolume->GetUnscaledBoxExtent().X);
	BuildOctree(RootNode, 0);

	UE_LOG(LogTemp, Warning, TEXT("======================================"));
	UE_LOG(LogTemp, Warning, TEXT("SVO memory complete!!"));
	UE_LOG(LogTemp, Warning, TEXT("======================================"));



}


void ASVOVolume::BuildOctree(TSharedPtr<FSVONode> Node, int32 CurrentDepth)
{
	FCollisionQueryParams QueryParams;
	// [핵심] 캐릭터는 지형이 아니므로 스캔 대상에서 제외한다.
	QueryParams.AddIgnoredActors(ActorsToIgnore);

	FCollisionShape BoxShape = FCollisionShape::MakeBox(FVector(Node->Extent));

	bool bHasObstacle = GetWorld()->OverlapAnyTestByChannel(
		Node->Center,
		FQuat::Identity,
		ECC_WorldStatic,
		BoxShape,
		QueryParams
	);

	if (!bHasObstacle)
	{
		Node->bIsBlocked = false;
		Node->bIsLeaf = true;
		return;
	}

	if (CurrentDepth >= MaxDepth)
	{
		Node->bIsBlocked = true;
		Node->bIsLeaf = true;
		return;
	}

	Node->bIsLeaf = false;
	Node->bIsBlocked = false;
	float ChildExtent = Node->Extent * 0.5f;

	for (int i = 0; i < 8; ++i)
	{
		FVector Offset(
			(i & 1) ? ChildExtent : -ChildExtent,
			(i & 2) ? ChildExtent : -ChildExtent,
			(i & 4) ? ChildExtent : -ChildExtent
		);

		TSharedPtr<FSVONode> ChildNode = MakeShared<FSVONode>(Node->Center + Offset, ChildExtent);
		Node->Children.Add(ChildNode);
		BuildOctree(ChildNode, CurrentDepth + 1);
	}
}

void ASVOVolume::RefreshIgnoredActors()
{
	ActorsToIgnore.Reset();
	// 스캔할 때 장애물로 세면 안 되는 동적 객체(캐릭터)를 다시 모아 온다.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), ActorsToIgnore);
	ActorsToIgnore.Add(this);
}

void ASVOVolume::RebuildRegion(const FBox& DirtyBounds)
{
	// 아직 트리가 없다면(최초 호출) 부분 재빌드가 성립하지 않으므로 전체 빌드로 대체한다.
	if (!RootNode.IsValid())
	{
		RefreshIgnoredActors();
		RootNode = MakeShared<FSVONode>(GetActorLocation(), BoundsVolume->GetUnscaledBoxExtent().X);
		BuildOctree(RootNode, 0);
		return;
	}

	if (!DirtyBounds.IsValid)
	{
		return;
	}

	// PCG로 지형이 바뀌었으므로, 무시할 동적 객체 목록도 최신 상태로 갱신한다.
	RefreshIgnoredActors();

	// 변경 영역(DirtyBounds)과 겹치는 노드만 다시 스캔한다.
	RebuildNodeRecursive(RootNode, 0, DirtyBounds);

	UE_LOG(LogTemp, Log, TEXT("[SVO] Partial rebuild done. Region=%s"), *DirtyBounds.ToString());
}

void ASVOVolume::RebuildRegionBox(FVector Center, FVector Extent)
{
	RebuildRegion(FBox(Center - Extent, Center + Extent));
}

void ASVOVolume::RebuildNodeRecursive(TSharedPtr<FSVONode> Node, int32 CurrentDepth, const FBox& DirtyBounds)
{
	if (!Node.IsValid()) return;

	// 이 노드가 차지하는 영역(AABB)
	const FBox NodeBox(Node->Center - FVector(Node->Extent), Node->Center + FVector(Node->Extent));

	// 변경 영역 밖이면 손대지 않는다. 여기서 서브트리 전체를 건너뛰는 이득이 나온다.
	if (!NodeBox.Intersect(DirtyBounds))
	{
		return;
	}

	if (Node->bIsLeaf)
	{
		// 이 리프만 다시 스캔한다. 새 장애물이 생겼다면 BuildOctree가 다시 쪼개거나 막는다.
		Node->Children.Reset();
		BuildOctree(Node, CurrentDepth);
		return;
	}

	// 중간 노드: 변경 영역과 겹치는 자식으로만 계속 내려간다.
	for (const TSharedPtr<FSVONode>& Child : Node->Children)
	{
		RebuildNodeRecursive(Child, CurrentDepth + 1, DirtyBounds);
	}
}

bool ASVOVolume::IsWalkable(const FVector& Location) const
{
	if (!RootNode.IsValid()) return false;
	TSharedPtr<FSVONode> CurrentNode = RootNode;
	while (!CurrentNode->bIsLeaf)
	{
		bool bFoundChild = false;
		for (const auto& Child : CurrentNode->Children)
		{
			if (FMath::Abs(Location.X - Child->Center.X) <= Child->Extent &&
				FMath::Abs(Location.Y - Child->Center.Y) <= Child->Extent &&
				FMath::Abs(Location.Z - Child->Center.Z) <= Child->Extent)
			{
				CurrentNode = Child;
				bFoundChild = true;
				break;
			}
		}
		if (!bFoundChild) return false;
	}
	return !CurrentNode->bIsBlocked;
}

bool ASVOVolume::SVORaycast(const FVector& Start, const FVector& End) const
{
	FVector Direction = End - Start;
	float Distance = Direction.Size();
	Direction.Normalize();

	float StepSize = (BoundsVolume->GetUnscaledBoxExtent().X * 2.0f) / FMath::Pow(2.0f, MaxDepth);
	StepSize *= 0.5f;

	float Traveled = 0.0f;
	while (Traveled < Distance)
	{
		FVector CurrentPoint = Start + (Direction * Traveled);
		if (!IsWalkable(CurrentPoint)) return true;
		Traveled += StepSize;
	}
	return false;
}


void ASVOVolume::DrawSVODebug() const
{
	if (RootNode.IsValid()) DrawNodeDebugRecursive(RootNode);
}

void ASVOVolume::ToggleSVODebug()
{
#if ENABLE_DRAW_DEBUG
	bIsDebugVisible = !bIsDebugVisible;

	// 이전에 그려 둔 디버그 라인과 박스를 화면에서 지운다.
	FlushPersistentDebugLines(GetWorld());

	if (bIsDebugVisible)
	{
		UE_LOG(LogTemp, Warning, TEXT("SVO debug rendering: ON"));
		DrawSVODebug(); // 토글을 켜는 순간 한 번 새로 그린다.
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SVO debug rendering: OFF"));
	}
#endif
}

void ASVOVolume::DrawNodeDebugRecursive(TSharedPtr<FSVONode> Node) const
{
	// 디버그를 그릴 때 유지 시간(LifeTime)은 -1(영구) 또는 충분히 긴 값으로 준다.
	float DebugLifeTime = 99999.0f;

	if (Node->bIsLeaf)
	{
		if (Node->bIsBlocked)
		{
			DrawDebugSolidBox(GetWorld(), Node->Center, FVector(Node->Extent), FColor(255, 0, 0, 100), false, DebugLifeTime, 0);
		}
		else
		{
			DrawDebugBox(GetWorld(), Node->Center, FVector(Node->Extent), FColor(0, 255, 0, 30), false, DebugLifeTime, 0, 2.0f);
		}
	}
	else
	{
		for (const auto& Child : Node->Children)
		{
			DrawNodeDebugRecursive(Child);
		}
	}
}