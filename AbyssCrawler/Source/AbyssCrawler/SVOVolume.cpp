#include "SVOVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

ASVOVolume::ASVOVolume()
{
	// Tick ���� ����
	PrimaryActorTick.bCanEverTick = false;

	BoundsVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsVolume"));
	RootComponent = BoundsVolume;
	BoundsVolume->SetBoxExtent(FVector(5000.0f, 5000.0f, 5000.0f));
}

void ASVOVolume::BeginPlay()
{
	Super::BeginPlay();

	// [New] �ʿ� �ִ� ��� ĳ����(���, �÷��̾� ��)�� ã�Ƽ� ���� ��Ͽ� �ֽ��ϴ�.
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
	// [�ٽ� �ذ�] �̸� ã�Ƶ� ��� ĳ���Ϳ� �ڽ��� ��ĵ ���̴����� ���������ϴ�!
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
	// ��ĵ �� �����ص� �ϴ� ���� ��ü(ĳ����)�� �ٽ� ��� ���´�.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), ActorsToIgnore);
	ActorsToIgnore.Add(this);
}

void ASVOVolume::RebuildRegion(const FBox& DirtyBounds)
{
	// ���� �ֻ�� ���尡 ���ٸ�(���� ���� ��) ��ü ����� ��ü.
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

	// PCG �� �������� ���� ��ü(ĳ����)�� �ٽ� ���ϵ��� ���� ��� ����.
	RefreshIgnoredActors();

	// ���� ���(DirtyBounds)�� ��ġ�� ��������� ��ĵ�Ѵ�.
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

	// �� ����� AABB
	const FBox NodeBox(Node->Center - FVector(Node->Extent), Node->Center + FVector(Node->Extent));

	// ���� ���� ���̸� �״�� ���� (�ٽ� ���: ��ü ���� �ƴ� �κ� ���)
	if (!NodeBox.Intersect(DirtyBounds))
	{
		return;
	}

	if (Node->bIsLeaf)
	{
		// �� ���Ǹ� �ٽ� ��ĵ. (���ο� ��ֹ��� �����ϸ� BuildOctree�� �ٽ� �ɰ��ų� ���´�)
		Node->Children.Reset();
		BuildOctree(Node, CurrentDepth);
		return;
	}

	// ���� ���: ���� ���� ��ġ�� �ڽ����� ��� ����
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

	// ������ �׷��� �ִ� ��� ����� ����/�ڽ��� ȭ�鿡�� ������ �� ����ϴ�.
	FlushPersistentDebugLines(GetWorld());

	if (bIsDebugVisible)
	{
		UE_LOG(LogTemp, Warning, TEXT("SVO debug rendering: ON"));
		DrawSVODebug(); // ���ð� ���� �� �� �� ���� �׸��ϴ�.
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SVO debug rendering: OFF"));
	}
#endif
}

void ASVOVolume::DrawNodeDebugRecursive(TSharedPtr<FSVONode> Node) const
{
	// ����׸� �׸� �� ���� �ð�(LifeTime)�� -1 (������) �Ǵ� ���� �� �ð����� �����մϴ�.
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