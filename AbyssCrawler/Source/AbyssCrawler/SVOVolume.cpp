#include "SVOVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

ASVOVolume::ASVOVolume()
{
	// Tick 연산 끄기
	PrimaryActorTick.bCanEverTick = false;

	BoundsVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsVolume"));
	RootComponent = BoundsVolume;
	BoundsVolume->SetBoxExtent(FVector(5000.0f, 5000.0f, 5000.0f));
}

void ASVOVolume::BeginPlay()
{
	Super::BeginPlay();

	// [New] 맵에 있는 모든 캐릭터(상어, 플레이어 등)를 찾아서 무시 목록에 넣습니다.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), ActorsToIgnore);

	// SVO 볼륨 자기 자신도 당연히 무시 목록에 추가
	ActorsToIgnore.Add(this);

	RootNode = MakeShared<FSVONode>(GetActorLocation(), BoundsVolume->GetUnscaledBoxExtent().X);
	BuildOctree(RootNode, 0);

	UE_LOG(LogTemp, Warning, TEXT("======================================"));
	UE_LOG(LogTemp, Warning, TEXT("SVO memory complete!!"));
	UE_LOG(LogTemp, Warning, TEXT("======================================"));



}


void ASVOVolume::BuildOctree(TSharedPtr<FSVONode> Node, int32 CurrentDepth)
{
	FCollisionQueryParams QueryParams;
	// [핵심 해결] 미리 찾아둔 모든 캐릭터와 자신을 스캔 레이더에서 지워버립니다!
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

	// 기존에 그려져 있던 모든 디버그 라인/박스를 화면에서 강제로 싹 지웁니다.
	FlushPersistentDebugLines(GetWorld());

	if (bIsDebugVisible)
	{
		UE_LOG(LogTemp, Warning, TEXT("SVO debug rendering: ON"));
		DrawSVODebug(); // 지시가 있을 때 단 한 번만 그립니다.
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SVO debug rendering: OFF"));
	}
#endif
}

void ASVOVolume::DrawNodeDebugRecursive(TSharedPtr<FSVONode> Node) const
{
	// 디버그를 그릴 때 지속 시간(LifeTime)을 -1 (영구적) 또는 아주 긴 시간으로 설정합니다.
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