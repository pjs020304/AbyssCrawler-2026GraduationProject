#include "SVOVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "SVOPathfinder.h"      // 벤치마크에서 FindPath / SmoothPath 를 직접 호출한다
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"

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

	// 전체 빌드에 걸린 시간을 재 둔다. 벤치마크에서 그대로 보고한다.
	LastFullBuildSeconds = RebuildWholeTreeTimed();

	UE_LOG(LogTemp, Warning, TEXT("======================================"));
	UE_LOG(LogTemp, Warning, TEXT("SVO memory complete!! (build %.1f ms)"), LastFullBuildSeconds * 1000.0);
	UE_LOG(LogTemp, Warning, TEXT("======================================"));

	// 커맨드라인에 -SVOBench 가 있을 때만 측정을 예약한다.
	// PCG 생성과 그에 따른 부분 재빌드가 끝난 뒤에 재야 실제 지형 위의 값이 나오므로
	// 곧바로 재지 않고 지연을 둔다. (-SVOBenchDelay=<초> 로 조정)
	if (FParse::Param(FCommandLine::Get(), TEXT("SVOBench")))
	{
		float BenchDelay = 20.0f;
		FParse::Value(FCommandLine::Get(), TEXT("SVOBenchDelay="), BenchDelay);

		int32 BenchTrials = 200;
		FParse::Value(FCommandLine::Get(), TEXT("SVOBenchTrials="), BenchTrials);

		FTimerHandle BenchTimer;
		GetWorldTimerManager().SetTimer(
			BenchTimer,
			FTimerDelegate::CreateWeakLambda(this, [this, BenchTrials]()
				{
					// ① 상어가 실제로 추적을 시작하는 거리 (SightRadius 3000 / LoseSight 3500)
					RunSVOBenchmark(BenchTrials, 600.0f, 3500.0f);
					// ② 오징어 (SightRadius 1500 / LoseSight 2000)
					RunSVOBenchmark(BenchTrials, 300.0f, 2000.0f);
					// ③ 볼륨 전체 무작위 — 킬 스위치가 도는 최악 조건
					RunSVOBenchmark(BenchTrials);
					// ④ MaxDepth 별 비교는 상어 조건으로 고정해 비교 가능하게 한다.
					RunSVODepthSweep(BenchTrials / 2, 600.0f, 3500.0f);

					// 자동 측정은 결과를 다 찍고 나면 프로세스를 끝낸다.
					UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 측정 종료. 프로세스를 닫는다."));
					FPlatformMisc::RequestExit(false);
				}),
			BenchDelay,
			false);

		UE_LOG(LogTemp, Warning, TEXT("[SVOBench] %.1f초 뒤 측정 예약됨 (시도 %d회)"), BenchDelay, BenchTrials);
	}



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

// ====================================================================================
//  측정용(벤치마크) 구현
//  게임 플레이 중에는 호출되지 않는다. -SVOBench 커맨드라인이나 블루프린트에서만 부른다.
// ====================================================================================

double ASVOVolume::RebuildWholeTreeTimed()
{
	const double StartSeconds = FPlatformTime::Seconds();

	RootNode = MakeShared<FSVONode>(GetActorLocation(), BoundsVolume->GetUnscaledBoxExtent().X);
	BuildOctree(RootNode, 0);

	return FPlatformTime::Seconds() - StartSeconds;
}

void ASVOVolume::CollectStats(const TSharedPtr<FSVONode>& Node, int32& OutInternal,
	int32& OutFreeLeaf, int32& OutBlockedLeaf) const
{
	if (!Node.IsValid())
	{
		return;
	}

	if (Node->bIsLeaf)
	{
		// 리프는 두 종류다. '비어 있어서 더 쪼갤 필요가 없는 것'과
		// '최대 깊이까지 쪼갰는데도 장애물이 남은 것'. 희소성은 앞쪽 수로 확인한다.
		if (Node->bIsBlocked)
		{
			++OutBlockedLeaf;
		}
		else
		{
			++OutFreeLeaf;
		}
		return;
	}

	++OutInternal;
	for (const TSharedPtr<FSVONode>& Child : Node->Children)
	{
		CollectStats(Child, OutInternal, OutFreeLeaf, OutBlockedLeaf);
	}
}

bool ASVOVolume::PickWalkablePoint(FRandomStream& Rng, FVector& OutPoint) const
{
	const FVector Center = GetActorLocation();
	const FVector Extent = BoundsVolume->GetUnscaledBoxExtent();

	// 비어 있는 지점이 나올 때까지 다시 뽑되, 상한을 두어 무한 루프를 막는다.
	for (int32 Attempt = 0; Attempt < 64; ++Attempt)
	{
		const FVector Candidate(
			Center.X + Rng.FRandRange(-Extent.X, Extent.X),
			Center.Y + Rng.FRandRange(-Extent.Y, Extent.Y),
			Center.Z + Rng.FRandRange(-Extent.Z, Extent.Z));

		if (IsWalkable(Candidate))
		{
			OutPoint = Candidate;
			return true;
		}
	}

	return false;
}

bool ASVOVolume::PickWalkablePointNear(FRandomStream& Rng, const FVector& Origin,
	float MinDistance, float MaxDistance, FVector& OutPoint) const
{
	const FVector Center = GetActorLocation();
	const FVector Extent = BoundsVolume->GetUnscaledBoxExtent();

	for (int32 Attempt = 0; Attempt < 128; ++Attempt)
	{
		// 방향은 구면에서 균일하게, 거리는 [Min, Max] 에서 뽑는다.
		const FVector Direction = Rng.VRand();
		const float Distance = Rng.FRandRange(MinDistance, MaxDistance);
		const FVector Candidate = Origin + Direction * Distance;

		// 볼륨 밖으로 나가면 옥트리가 답할 수 없는 좌표다.
		if (FMath::Abs(Candidate.X - Center.X) > Extent.X ||
			FMath::Abs(Candidate.Y - Center.Y) > Extent.Y ||
			FMath::Abs(Candidate.Z - Center.Z) > Extent.Z)
		{
			continue;
		}

		if (IsWalkable(Candidate))
		{
			OutPoint = Candidate;
			return true;
		}
	}

	return false;
}

void ASVOVolume::LogDurations(const TCHAR* Label, TArray<double>& Milliseconds)
{
	if (Milliseconds.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SVOBench] %s | 표본 없음"), Label);
		return;
	}

	// 중앙값과 p95 를 뽑기 위해 정렬한다. 그래서 배열을 참조로 받는다.
	Milliseconds.Sort();

	double Sum = 0.0;
	for (const double Value : Milliseconds)
	{
		Sum += Value;
	}

	const int32 Count = Milliseconds.Num();
	const double Mean = Sum / Count;
	const double Median = Milliseconds[Count / 2];
	const int32 P95Index = FMath::Clamp(FMath::FloorToInt32(Count * 0.95f), 0, Count - 1);

	UE_LOG(LogTemp, Warning,
		TEXT("[SVOBench] %s | n=%d  min %.3f  mean %.3f  median %.3f  p95 %.3f  max %.3f (ms)"),
		Label, Count, Milliseconds[0], Mean, Median, Milliseconds[P95Index], Milliseconds.Last());
}

void ASVOVolume::RunSVOBenchmark(int32 NumTrials, float MinDistance, float MaxDistance)
{
	if (!RootNode.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SVOBench] 옥트리가 아직 없다. 측정을 건너뛴다."));
		return;
	}

	// ---- 1. 옥트리 자체의 크기 ----
	int32 Internal = 0;
	int32 FreeLeaf = 0;
	int32 BlockedLeaf = 0;
	CollectStats(RootNode, Internal, FreeLeaf, BlockedLeaf);

	const int32 TotalNodes = Internal + FreeLeaf + BlockedLeaf;

	// 메모리는 추정치다. 노드 본체 + 내부 노드마다 자식 8개를 담는 배열 할당만 센다.
	// MakeShared 의 참조 카운터 블록과 할당자 오버헤드는 포함되지 않는다.
	const SIZE_T ApproxBytes =
		static_cast<SIZE_T>(TotalNodes) * sizeof(FSVONode)
		+ static_cast<SIZE_T>(Internal) * 8 * sizeof(TSharedPtr<FSVONode>);

	const FVector VolumeExtent = BoundsVolume->GetUnscaledBoxExtent();

	UE_LOG(LogTemp, Warning, TEXT("=========== SVO Benchmark ==========="));
	if (MaxDistance > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 측정 조건: 시작점에서 %.0f~%.0f uu 떨어진 목표"), MinDistance, MaxDistance);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 측정 조건: 볼륨 전체 무작위 (최악 조건)"));
	}
	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 볼륨 반경 %s / MaxDepth %d / 최소 복셀 %.1f uu"),
		*VolumeExtent.ToString(), MaxDepth, (VolumeExtent.X * 2.0f) / FMath::Pow(2.0f, static_cast<float>(MaxDepth)));
	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 노드 %d개 (내부 %d / 빈 리프 %d / 막힌 리프 %d)"),
		TotalNodes, Internal, FreeLeaf, BlockedLeaf);
	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 노드 1개 %d바이트 / 트리 추정 %.2f MB"),
		static_cast<int32>(sizeof(FSVONode)), ApproxBytes / (1024.0 * 1024.0));
	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 직전 전체 빌드 %.1f ms"), LastFullBuildSeconds * 1000.0);

	// ---- 2. A* 1회 비용 ----
	// 시드를 고정해 다시 돌려도 같은 표본이 나오게 한다.
	FRandomStream Rng(1337);

	TArray<double> FindMs;
	TArray<double> SmoothMs;
	TArray<double> TotalMs;
	FindMs.Reserve(NumTrials);
	SmoothMs.Reserve(NumTrials);
	TotalMs.Reserve(NumTrials);

	int32 Sampled = 0;
	int32 PathFound = 0;
	int32 ReachedTarget = 0;
	double SumDistance = 0.0;
	double SumRawPoints = 0.0;
	double SumSmoothPoints = 0.0;

	for (int32 Trial = 0; Trial < NumTrials; ++Trial)
	{
		FVector StartLoc = FVector::ZeroVector;
		FVector TargetLoc = FVector::ZeroVector;
		if (!PickWalkablePoint(Rng, StartLoc))
		{
			continue;
		}

		const bool bPicked = (MaxDistance > 0.0f)
			? PickWalkablePointNear(Rng, StartLoc, MinDistance, MaxDistance, TargetLoc)
			: PickWalkablePoint(Rng, TargetLoc);

		if (!bPicked)
		{
			continue;
		}

		// 인덱스 변환은 BTTask_SmoothChasePlayer 와 같은 방식이어야 조건이 같아진다.
		const FIntVector StartIndex(
			FMath::RoundToInt(StartLoc.X / BenchVoxelSize),
			FMath::RoundToInt(StartLoc.Y / BenchVoxelSize),
			FMath::RoundToInt(StartLoc.Z / BenchVoxelSize));

		const FIntVector TargetIndex(
			FMath::RoundToInt(TargetLoc.X / BenchVoxelSize),
			FMath::RoundToInt(TargetLoc.Y / BenchVoxelSize),
			FMath::RoundToInt(TargetLoc.Z / BenchVoxelSize));

		if (StartIndex == TargetIndex)
		{
			continue;
		}

		++Sampled;
		SumDistance += FVector::Dist(StartLoc, TargetLoc);

		const double T0 = FPlatformTime::Seconds();
		TArray<FVector> RawPath = FSVOPathfinder::FindPath(
			this, StartIndex, StartLoc, TargetIndex, TargetLoc, BenchVoxelSize);
		const double T1 = FPlatformTime::Seconds();

		TArray<FVector> SmoothedPath;
		if (RawPath.Num() > 0)
		{
			SmoothedPath = FSVOPathfinder::SmoothPath(this, RawPath);
		}
		const double T2 = FPlatformTime::Seconds();

		FindMs.Add((T1 - T0) * 1000.0);
		SmoothMs.Add((T2 - T1) * 1000.0);
		TotalMs.Add((T2 - T0) * 1000.0);

		if (RawPath.Num() > 0)
		{
			++PathFound;
			// 마지막 점이 목표 근처면 완주, 아니면 킬 스위치(2000회)에 걸려
			// 부분 경로를 돌려준 것이다.
			if (FVector::Dist(RawPath.Last(), TargetLoc) <= BenchVoxelSize * 1.5f)
			{
				++ReachedTarget;
			}
			SumRawPoints += RawPath.Num();
			SumSmoothPoints += SmoothedPath.Num();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 표본 %d회 / 경로 반환 %d회 / 평균 직선거리 %.0f uu"),
		Sampled, PathFound, Sampled > 0 ? (SumDistance / Sampled) : 0.0);
	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 목표까지 완주 %d회 / 킬스위치(2000회) 걸려 부분 경로 %d회"),
		ReachedTarget, PathFound - ReachedTarget);

	if (PathFound > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 평활화 전 %.1f점 -> 평활화 후 %.1f점"),
			SumRawPoints / PathFound, SumSmoothPoints / PathFound);
	}

	LogDurations(TEXT("FindPath   (A* tamsaek)"), FindMs);
	LogDurations(TEXT("SmoothPath (gyeongro pyeonghwalhwa)"), SmoothMs);
	LogDurations(TEXT("HAPGYE = donggi silhaeng si frame jeongji"), TotalMs);
	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] 참고: 60fps 프레임 예산은 16.667 ms"));
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
}

void ASVOVolume::RunSVODepthSweep(int32 NumTrials, float MinDistance, float MaxDistance)
{
	const int32 OriginalMaxDepth = MaxDepth;

	UE_LOG(LogTemp, Warning, TEXT("=========== SVO Depth Sweep ==========="));

	for (int32 Depth = 4; Depth <= 6; ++Depth)
	{
		MaxDepth = Depth;
		LastFullBuildSeconds = RebuildWholeTreeTimed();

		UE_LOG(LogTemp, Warning, TEXT("---------- MaxDepth = %d ----------"), Depth);
		RunSVOBenchmark(NumTrials, MinDistance, MaxDistance);
	}

	// 측정 때문에 게임 상태가 달라지면 안 되므로 원래 깊이로 되돌려 다시 빌드한다.
	MaxDepth = OriginalMaxDepth;
	LastFullBuildSeconds = RebuildWholeTreeTimed();

	UE_LOG(LogTemp, Warning, TEXT("[SVOBench] MaxDepth 를 원래 값(%d)으로 되돌리고 재빌드했다."), OriginalMaxDepth);
	UE_LOG(LogTemp, Warning, TEXT("======================================="));
}
