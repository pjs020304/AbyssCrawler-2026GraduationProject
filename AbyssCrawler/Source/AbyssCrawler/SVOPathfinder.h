#pragma once

#include "CoreMinimal.h"
#include "Algo/Reverse.h" // Algo::Reverse 사용을 위해 필수
#include "SVOVolume.h"


// 복셀의 3D 인덱스(좌표)를 해시 맵에서 키(Key)로 쓰기 위한 해시 함수
FORCEINLINE uint32 GetTypeHash(const FIntVector& Vector)
{
	return FCrc::MemCrc32(&Vector, sizeof(FIntVector));
}

// A* 탐색에 사용될 노드 구조체
struct FSVOPathNode
{
	FIntVector GridIndex;     // 복셀의 3차원 배열 인덱스 (X, Y, Z)
	FVector WorldLocation;    // 실제 월드 좌표 (복셀의 중심점)

	float GCost;              // 시작점부터 여기까지 오는 데 걸린 실제 비용
	float HCost;              // (휴리스틱) 여기서부터 목적지까지의 예상 비용

	FSVOPathNode* Parent;     // 경로를 역추적하기 위한 부모 노드 포인터

	FSVOPathNode(FIntVector InIndex, FVector InLoc)
		: GridIndex(InIndex), WorldLocation(InLoc), GCost(0), HCost(0), Parent(nullptr) {
	}

	// F = G + H. A* 알고리즘은 항상 FCost가 가장 낮은 노드를 우선 탐색합니다.
	float GetFCost() const { return GCost + HCost; }

	// 이진 힙(Min-Heap) 정렬을 위한 연산자 오버로딩 (FCost가 작을수록 우선순위 높음)
	bool operator<(const FSVOPathNode& Other) const
	{
		return GetFCost() < Other.GetFCost();
	}
};

class FSVOPathfinder
{
public:
	// 26방향 이웃 오프셋 미리 계산 (상하좌우, 대각선 모두 포함)
	static const TArray<FIntVector>& Get26Directions()
	{
		static const TArray<FIntVector> Directions = []()
			{
				TArray<FIntVector> Result;
				Result.Reserve(26);

				for (int32 X = -1; X <= 1; ++X)
				{
					for (int32 Y = -1; Y <= 1; ++Y)
					{
						for (int32 Z = -1; Z <= 1; ++Z)
						{
							if (X == 0 && Y == 0 && Z == 0)
							{
								continue;
							}

							Result.Add(FIntVector(X, Y, Z));
						}
					}
				}

				return Result;
			}();

		return Directions;
	}

	// 3D 체비쇼프 거리 휴리스틱 함수 계산
	static float CalculateChebyshevHeuristic(const FIntVector& NodeA, const FIntVector& NodeB)
	{
		int32 Dx = FMath::Abs(NodeA.X - NodeB.X);
		int32 Dy = FMath::Abs(NodeA.Y - NodeB.Y);
		int32 Dz = FMath::Abs(NodeA.Z - NodeB.Z);

		// 3개 축의 차이 중 가장 큰 값을 반환
		return static_cast<float>(FMath::Max3(Dx, Dy, Dz));
	}
	static TArray<FVector> FindPath(ASVOVolume* SVOData, FIntVector StartIndex, FVector StartLoc, FIntVector TargetIndex, FVector TargetLoc, float VoxelSize);

	static TArray<FVector> SmoothPath(ASVOVolume* SVOData, const TArray<FVector>& OriginalPath);

};

// 가상의 함수 선언: 실제 SVO 데이터에서 해당 인덱스의 복셀이 비어있는지(이동 가능한지) 확인하는 함수
// bool IsVoxelWalkable(FIntVector GridIndex); 

inline TArray<FVector> FSVOPathfinder::FindPath(ASVOVolume* SVOData, FIntVector StartIndex, FVector StartLoc, FIntVector TargetIndex, FVector TargetLoc, float VoxelSize)
{
	TArray<FVector> OutPath;

	// SVO 데이터가 없으면 길찾기 취소
	if (!SVOData || StartIndex == TargetIndex) return OutPath;

	TMap<FIntVector, TSharedPtr<FSVOPathNode>> AllNodes;
	TArray<TSharedPtr<FSVOPathNode>> OpenList;
	TSet<FIntVector> ClosedList;

	// 무한 루프 및 프레임 드랍 방지를 위한 킬 스위치
	int32 MaxIterations = 2000;
	int32 CurrentIterations = 0;
	TSharedPtr<FSVOPathNode> BestNodeSoFar = nullptr;
	float ClosestDist = MAX_FLT;

	TSharedPtr<FSVOPathNode> StartNode = MakeShared<FSVOPathNode>(StartIndex, StartLoc);
	AllNodes.Add(StartIndex, StartNode);
	OpenList.HeapPush(StartNode, [](const TSharedPtr<FSVOPathNode>& A, const TSharedPtr<FSVOPathNode>& B) {
		return *A < *B;
		});

	while (OpenList.Num() > 0)
	{
		// 안전장치: 연산 횟수가 한계에 달하면 강제 종료
		if (++CurrentIterations > MaxIterations)
		{
			UE_LOG(LogTemp, Warning, TEXT("A* overwork(partition path return)"));
			// 완벽한 길은 못 찾았지만, 목적지와 가장 가까워진 노드에서부터 경로를 역추적하여 반환합니다.
			if (BestNodeSoFar.IsValid())
			{
				FSVOPathNode* TraceNode = BestNodeSoFar.Get();
				while (TraceNode != nullptr)
				{
					OutPath.Add(TraceNode->WorldLocation);
					TraceNode = TraceNode->Parent;
				}
				Algo::Reverse(OutPath);
			}
			return OutPath;
		}

		TSharedPtr<FSVOPathNode> CurrentNode;
		OpenList.HeapPop(CurrentNode, [](const TSharedPtr<FSVOPathNode>& A, const TSharedPtr<FSVOPathNode>& B) {
			return *A < *B;
			});

		// 목적지와 가장 가까운 노드 갱신 (킬 스위치 발동 시 사용)
		float DistToTarget = FVector::Dist(CurrentNode->WorldLocation, TargetLoc);
		if (DistToTarget < ClosestDist)
		{
			ClosestDist = DistToTarget;
			BestNodeSoFar = CurrentNode;
		}

		if (CurrentNode->GridIndex == TargetIndex)
		{
			FSVOPathNode* TraceNode = CurrentNode.Get();
			while (TraceNode != nullptr)
			{
				OutPath.Add(TraceNode->WorldLocation);
				TraceNode = TraceNode->Parent;
			}
			Algo::Reverse(OutPath);
			return OutPath;
		}

		ClosedList.Add(CurrentNode->GridIndex);

		for (const FIntVector& Dir : Get26Directions())
		{
			FIntVector NeighborIndex = CurrentNode->GridIndex + Dir;
			if (ClosedList.Contains(NeighborIndex)) continue;

			// [핵심 해결] 이웃 복셀의 실제 월드 좌표 계산 (VoxelSize 적용)
			FVector NeighborWorldLoc = CurrentNode->WorldLocation + (FVector(Dir) * VoxelSize);

			// [마침내 적용된 3D SVO 충돌 검사!] 
			// SVO 지도를 뒤져서 이 좌표가 벽(땅속/암초)이라면 무시하고 다른 방향을 찾습니다!
			if (!SVOData->IsWalkable(NeighborWorldLoc)) continue;

			float TentativeGCost = CurrentNode->GCost + 1.0f;

			TSharedPtr<FSVOPathNode> NeighborNode;
			bool bIsNewNode = false;

			if (AllNodes.Contains(NeighborIndex))
			{
				NeighborNode = AllNodes[NeighborIndex];
			}
			else
			{
				NeighborNode = MakeShared<FSVOPathNode>(NeighborIndex, NeighborWorldLoc);
				AllNodes.Add(NeighborIndex, NeighborNode);
				bIsNewNode = true;
			}

			if (bIsNewNode || TentativeGCost < NeighborNode->GCost)
			{
				NeighborNode->Parent = CurrentNode.Get();
				NeighborNode->GCost = TentativeGCost;
				NeighborNode->HCost = CalculateChebyshevHeuristic(NeighborIndex, TargetIndex);

				if (bIsNewNode)
				{
					OpenList.HeapPush(NeighborNode, [](const TSharedPtr<FSVOPathNode>& A, const TSharedPtr<FSVOPathNode>& B) { return *A < *B; });
				}
				else
				{
					OpenList.Heapify([](const TSharedPtr<FSVOPathNode>& A, const TSharedPtr<FSVOPathNode>& B) { return *A < *B; });
				}
			}
		}
	}

	return OutPath;
}

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"

inline TArray<FVector> FSVOPathfinder::SmoothPath(ASVOVolume* SVOData, const TArray<FVector>& OriginalPath)
{
	if (OriginalPath.Num() <= 2) return OriginalPath;

	TArray<FVector> SmoothedPath;
	SmoothedPath.Add(OriginalPath[0]);

	int32 CheckPointIndex = 0;
	int32 CurrentIndex = 1;

	while (CurrentIndex < OriginalPath.Num() - 1)
	{
		FVector Start = OriginalPath[CheckPointIndex];
		FVector Target = OriginalPath[CurrentIndex + 1];

		// 무거운 물리 엔진 Sweep 대신, SVO 메모리 기반 초고속 레이캐스트 사용!
		bool bHit = SVOData->SVORaycast(Start, Target);

		if (!bHit)
		{
			CurrentIndex++;
		}
		else
		{
			SmoothedPath.Add(OriginalPath[CurrentIndex]);
			CheckPointIndex = CurrentIndex;
			CurrentIndex++;
		}
	}
	SmoothedPath.Add(OriginalPath.Last());
	return SmoothedPath;
}