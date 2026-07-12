// Fill out your copyright notice in the Description page of Project Settings.

#include "AbyssPCGManager.h"
#include "SVOVolume.h"
#include "AbyssItemBase.h"

#include "PCGComponent.h"
#include "Data/PCGBasePointData.h"       // 아이템 배치: Output 포인트 읽기
#include "EngineUtils.h"                 // TActorIterator
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AAbyssPCGManager::AAbyssPCGManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// 시드를 리플리케이트해야 하므로 액터 리플리케이션 활성화.
	bReplicates = true;
	bAlwaysRelevant = true;          // 맵 전체에 영향을 주므로 항상 관련
	AccumulatedDirtyBounds.Init();   // Invalid 상태로 시작
}

void AAbyssPCGManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssPCGManager, ReplicatedSeed);
}

void AAbyssPCGManager::BeginPlay()
{
	Super::BeginPlay();

	// SVO 볼륨 자동 탐색 (미지정 시)
	if (!TargetSVOVolume)
	{
		TargetSVOVolume = Cast<ASVOVolume>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ASVOVolume::StaticClass()));
	}

	if (HasAuthority())
	{
		// [서버] 이번 판의 시드를 결정하고 리플리케이트한다.
		// 0은 "미설정"으로 취급하므로 1 이상으로 뽑는다.
		ReplicatedSeed = FMath::RandRange(1, MAX_int32 - 1);

		// 서버 자신은 OnRep이 호출되지 않으므로 직접 생성 시작.
		// (서버의 SVO가 있어야 AI 길찾기가 산호를 회피한다)
		StartGeneration(ReplicatedSeed);
	}
	else
	{
		// [클라] 시드가 이미 도착해 있으면 즉시 시작, 아니면 OnRep_Seed를 기다린다.
		if (ReplicatedSeed != 0)
		{
			StartGeneration(ReplicatedSeed);
		}
	}
}

void AAbyssPCGManager::OnRep_Seed()
{
	// 클라이언트가 서버 시드를 수신한 시점.
	if (ReplicatedSeed != 0)
	{
		StartGeneration(ReplicatedSeed);
	}
}

void AAbyssPCGManager::StartGeneration(int32 InSeed)
{
	if (bGenerationStarted)
	{
		return;
	}
	bGenerationStarted = true;

	// ── 장식(산호/암초) PCG: 모든 머신에서 동일 시드로 로컬 생성 ──
	TArray<UPCGComponent*> Components = CollectPCGComponents();
	PendingComponents = Components.Num();
	AccumulatedDirtyBounds.Init();

	if (PendingComponents == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbyssPCG] 장식 PCG 컴포넌트를 찾지 못했습니다. SVO 재빌드를 건너뜁니다."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[AbyssPCG] Seed=%d 로 장식 PCG %d개 생성 시작 (Authority=%d)"),
			InSeed, PendingComponents, HasAuthority());

		for (UPCGComponent* Comp : Components)
		{
			if (!Comp) { continue; }

			// 1) 모든 머신이 동일 시드로 → 동일 배치 (결정론 동기화의 핵심)
			Comp->Seed = InSeed;

			// 2) 완료 시점을 잡기 위해 델리게이트 바인딩 (연산 순서 보장)
			Comp->OnPCGGraphGeneratedDelegate.AddUObject(this, &AAbyssPCGManager::HandlePCGGenerated);

			// 3) 강제 재생성 (bForce=true: 이전 결과를 정리하고 새 시드로 다시 생성)
			Comp->Generate(/*bForce=*/true);
		}
	}

	// ── 아이템 PCG: 서버만 생성. 완료 시 리플리케이트 아이템 액터를 스폰한다 ──
	// (클라 로컬 스폰은 권한/동기화가 깨지므로 금지. 클라에는 액터 리플리케이션으로 전달됨)
	if (HasAuthority())
	{
		StartItemGeneration();
	}
}

void AAbyssPCGManager::HandlePCGGenerated(UPCGComponent* InComponent)
{
	if (InComponent)
	{
		// 재진입 방지: 이 컴포넌트의 델리게이트는 해제
		InComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);

		// 이 PCG가 실제로 스폰한 콘텐츠의 바운드를 누적.
		// PCG가 만든 ISM/스폰 액터는 소유 액터에 붙으므로 소유 액터의 전체 바운드를 사용한다.
		if (AActor* OwnerActor = InComponent->GetOwner())
		{
			const FBox OwnerBox = OwnerActor->GetComponentsBoundingBox(/*bNonColliding=*/true);
			if (OwnerBox.IsValid)
			{
				AccumulatedDirtyBounds += OwnerBox;
			}
		}
	}

	// 아직 끝나지 않은 PCG가 남아 있으면 대기.
	if (--PendingComponents > 0)
	{
		return;
	}

	// ── 여기 도달 = "모든 PCG 생성 완료" ──
	// 이제서야 SVO 변경구역을 전달하고 부분 재빌드한다. (순서 보장)
	if (TargetSVOVolume && AccumulatedDirtyBounds.IsValid)
	{
		const FBox PaddedBounds = AccumulatedDirtyBounds.ExpandBy(DirtyBoundsPadding);
		TargetSVOVolume->RebuildRegion(PaddedBounds);

		UE_LOG(LogTemp, Log, TEXT("[AbyssPCG] 전체 PCG 완료 → SVO 부분 재빌드 요청."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbyssPCG] SVO 볼륨이 없거나 변경 영역이 비어 재빌드를 건너뜁니다."));
	}
}

TArray<UPCGComponent*> AAbyssPCGManager::CollectPCGComponents() const
{
	TArray<UPCGComponent*> Result;

	// 아이템 배치 전용 액터는 장식 경로에서 제외
	// (클라가 아이템 PCG를 헛돌리거나 서버에서 이중 바인딩되는 것 방지)
	TSet<const AActor*> ItemActorSet;
	for (const TObjectPtr<AActor>& ItemActor : ItemPCGActors)
	{
		if (ItemActor) { ItemActorSet.Add(ItemActor); }
	}

	if (PCGActors.Num() > 0)
	{
		// 지정된 액터에서만 수집
		for (const TObjectPtr<AActor>& Actor : PCGActors)
		{
			if (!Actor || ItemActorSet.Contains(Actor)) { continue; }
			TArray<UPCGComponent*> Comps;
			Actor->GetComponents<UPCGComponent>(Comps);
			Result.Append(Comps);
		}
	}
	else
	{
		// 미지정 시 레벨 전체에서 UPCGComponent를 가진 액터를 자동 수집
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (ItemActorSet.Contains(*It)) { continue; }
			TArray<UPCGComponent*> Comps;
			It->GetComponents<UPCGComponent>(Comps);
			Result.Append(Comps);
		}
	}

	return Result;
}

// ──────────────────────────────────────────────────────────────
// 아이템 배치 경로 (서버 전용)
// ──────────────────────────────────────────────────────────────
void AAbyssPCGManager::StartItemGeneration()
{
	TArray<UPCGComponent*> ItemComps = CollectItemPCGComponents();
	if (ItemComps.Num() == 0)
	{
		return; // 아이템 PCG 미사용
	}

	if (ItemSpawnTable.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbyssPCG] ItemPCGActors는 있지만 ItemSpawnTable이 비어 있어 아이템을 스폰할 수 없습니다."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AbyssPCG] 아이템 PCG %d개 생성 시작 (서버 전용)"), ItemComps.Num());

	for (UPCGComponent* Comp : ItemComps)
	{
		if (!Comp) { continue; }

		Comp->Seed = ReplicatedSeed;
		Comp->OnPCGGraphGeneratedDelegate.AddUObject(this, &AAbyssPCGManager::HandleItemPCGGenerated);
		Comp->Generate(/*bForce=*/true);
	}
}

void AAbyssPCGManager::HandleItemPCGGenerated(UPCGComponent* InComponent)
{
	if (!InComponent) { return; }
	InComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);

	// 그래프 Output 노드에 연결된 모든 포인트 데이터를 후보 위치로 수집
	TArray<FTransform> Candidates;
	for (const FPCGTaggedData& Tagged : InComponent->GetGeneratedGraphOutput().TaggedData)
	{
		if (const UPCGBasePointData* Points = Cast<UPCGBasePointData>(Tagged.Data))
		{
			const int32 NumPoints = Points->GetNumPoints();
			Candidates.Reserve(Candidates.Num() + NumPoints);
			for (int32 i = 0; i < NumPoints; ++i)
			{
				Candidates.Add(Points->GetTransform(i));
			}
		}
	}

	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbyssPCG] 아이템 PCG '%s'의 Output에 포인트가 없습니다. 그래프에서 포인트를 Output 노드까지 연결했는지 확인하세요."),
			*InComponent->GetOwner()->GetName());
		return;
	}

	// 시드 기반 셔플: 같은 시드면 같은 선별 결과 (컴포넌트별로 다른 패턴이 되도록 이름 해시 결합)
	const AActor* OwnerActor = InComponent->GetOwner();
	FRandomStream Rng(ReplicatedSeed ^ static_cast<int32>(GetTypeHash(OwnerActor ? OwnerActor->GetFName() : InComponent->GetFName())));

	for (int32 i = Candidates.Num() - 1; i > 0; --i)   // Fisher-Yates
	{
		Candidates.Swap(i, Rng.RandRange(0, i));
	}

	const int32 SpawnCount = FMath::Min(Candidates.Num(), MaxItemSpawnCount);
	int32 SpawnedCount = 0;

	for (int32 i = 0; i < SpawnCount; ++i)
	{
		const TSubclassOf<AAbyssItemBase> PickedClass = PickWeightedItemClass(Rng);
		if (!PickedClass) { continue; }

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		const FVector SpawnLocation = Candidates[i].GetLocation() + FVector(0.f, 0.f, ItemSpawnZOffset);
		const FRotator SpawnRotation(0.f, Rng.FRandRange(0.f, 360.f), 0.f);

		if (GetWorld()->SpawnActor<AAbyssItemBase>(PickedClass, SpawnLocation, SpawnRotation, Params))
		{
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[AbyssPCG] 아이템 스폰 완료: 후보 %d개 중 %d개 스폰 ('%s')"),
		Candidates.Num(), SpawnedCount, OwnerActor ? *OwnerActor->GetName() : TEXT("?"));
}

TArray<UPCGComponent*> AAbyssPCGManager::CollectItemPCGComponents() const
{
	TArray<UPCGComponent*> Result;
	for (const TObjectPtr<AActor>& Actor : ItemPCGActors)
	{
		if (!Actor) { continue; }
		TArray<UPCGComponent*> Comps;
		Actor->GetComponents<UPCGComponent>(Comps);
		Result.Append(Comps);
	}
	return Result;
}

TSubclassOf<AAbyssItemBase> AAbyssPCGManager::PickWeightedItemClass(FRandomStream& Rng) const
{
	float TotalWeight = 0.0f;
	for (const FAbyssPCGItemEntry& Entry : ItemSpawnTable)
	{
		if (Entry.ItemClass && Entry.Weight > 0.0f)
		{
			TotalWeight += Entry.Weight;
		}
	}
	if (TotalWeight <= 0.0f) { return nullptr; }

	float Roll = Rng.FRandRange(0.0f, TotalWeight);
	for (const FAbyssPCGItemEntry& Entry : ItemSpawnTable)
	{
		if (!Entry.ItemClass || Entry.Weight <= 0.0f) { continue; }
		Roll -= Entry.Weight;
		if (Roll <= 0.0f) { return Entry.ItemClass; }
	}
	return nullptr;
}
