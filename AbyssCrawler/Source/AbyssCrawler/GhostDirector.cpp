#include "GhostDirector.h"

#include "GhostCreature.h"
#include "AbyssDiverCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

AGhostDirector::AGhostDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGhostDirector::BeginPlay()
{
	Super::BeginPlay();
}

void AGhostDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DespawnAllGhosts();
	Super::EndPlay(EndPlayReason);
}

void AGhostDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 모든 판정은 서버 권위.
	if (!HasAuthority())
	{
		return;
	}

	TArray<AAbyssDiverCharacter*> Divers;
	GatherAliveDivers(Divers);

	const bool bAllSwim = AreAllSwimming(Divers);

	if (!bAllSwim)
	{
		// 팀 전체가 수영 중이 아니면: 타이머 리셋 + 유령 전부 디스폰 + 디버프 해제.
		AllSwimmingTime = 0.f;
		if (ActiveGhosts.Num() > 0)
		{
			DespawnAllGhosts();
		}
		for (AAbyssDiverCharacter* Diver : Divers)
		{
			Diver->GhostHauntTime = 0.f;
			Diver->SetGhostHauntStage(0);
		}
		return;
	}

	AllSwimmingTime += DeltaTime;

	UpdateSpawning(Divers);
	UpdateHauntAndCatch(DeltaTime, Divers);
}

void AGhostDirector::GatherAliveDivers(TArray<AAbyssDiverCharacter*>& OutDivers) const
{
	OutDivers.Reset();
	for (TActorIterator<AAbyssDiverCharacter> It(GetWorld()); It; ++It)
	{
		AAbyssDiverCharacter* Diver = *It;
		if (IsValid(Diver) && !Diver->bIsDead)
		{
			OutDivers.Add(Diver);
		}
	}
}

bool AGhostDirector::AreAllSwimming(const TArray<AAbyssDiverCharacter*>& Divers) const
{
	if (Divers.Num() == 0)
	{
		return false;
	}

	for (const AAbyssDiverCharacter* Diver : Divers)
	{
		const UCharacterMovementComponent* Move = Diver->GetCharacterMovement();
		if (!Move || !Move->IsSwimming())
		{
			return false;
		}
	}
	return true;
}

void AGhostDirector::UpdateSpawning(const TArray<AAbyssDiverCharacter*>& Divers)
{
	if (!*GhostClass)
	{
		return; // 스폰할 클래스 미지정
	}

	if (AllSwimmingTime < SpawnAfterSeconds)
	{
		return;
	}

	PruneGhosts();

	// 300초에 1기, 이후 SpawnInterval마다 1기씩 증가(최대 MaxGhosts).
	int32 Desired = 1 + FMath::FloorToInt((AllSwimmingTime - SpawnAfterSeconds) / FMath::Max(SpawnInterval, 1.f));
	Desired = FMath::Clamp(Desired, 1, MaxGhosts);

	while (ActiveGhosts.Num() < Desired)
	{
		FVector SpawnLoc;
		if (!FindSpawnLocation(Divers, SpawnLoc))
		{
			break; // 적절한 위치를 못 찾으면 이번 틱은 보류
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGhostCreature* Ghost = GetWorld()->SpawnActor<AGhostCreature>(
			GhostClass, SpawnLoc, FRotator::ZeroRotator, Params);
		if (!Ghost)
		{
			break;
		}
		ActiveGhosts.Add(Ghost);
	}
}

bool AGhostDirector::FindSpawnLocation(const TArray<AAbyssDiverCharacter*>& Divers, FVector& OutLocation) const
{
	if (Divers.Num() == 0)
	{
		return false;
	}

	const float MinDistSq = FMath::Square(SpawnMinPlayerDistance);

	for (int32 Try = 0; Try < FMath::Max(SpawnTryCount, 1); ++Try)
	{
		// 임의의 플레이어를 기준으로 임의 방향/거리에 후보 생성.
		const AAbyssDiverCharacter* Anchor = Divers[FMath::RandRange(0, Divers.Num() - 1)];
		const FVector Dir = FMath::VRand();
		const float Dist = SpawnMinPlayerDistance * FMath::FRandRange(1.0f, 1.3f);
		const FVector Candidate = Anchor->GetActorLocation() + Dir * Dist;

		// 모든 플레이어로부터 최소 거리 이상인지 확인.
		bool bFarEnough = true;
		for (const AAbyssDiverCharacter* Diver : Divers)
		{
			if (FVector::DistSquared(Candidate, Diver->GetActorLocation()) < MinDistSq)
			{
				bFarEnough = false;
				break;
			}
		}

		if (bFarEnough)
		{
			OutLocation = Candidate;
			return true;
		}
	}

	return false;
}

void AGhostDirector::UpdateHauntAndCatch(float DeltaTime, const TArray<AAbyssDiverCharacter*>& Divers)
{
	const float HauntRadiusSq = FMath::Square(HauntRadius);
	const float CatchRadiusSq = FMath::Square(CatchRadius);

	for (AAbyssDiverCharacter* Diver : Divers)
	{
		// 가장 가까운 유령까지의 거리.
		float NearestSq = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<AGhostCreature>& GhostPtr : ActiveGhosts)
		{
			if (AGhostCreature* Ghost = GhostPtr.Get())
			{
				NearestSq = FMath::Min(NearestSq,
					FVector::DistSquared(Diver->GetActorLocation(), Ghost->GetActorLocation()));
			}
		}

		// 포획: 접촉 시 즉시 즉사 진행.
		if (NearestSq <= CatchRadiusSq)
		{
			Diver->CaughtByGhost();
			continue;
		}

		// 근접 시간 누적 / 회복.
		if (NearestSq <= HauntRadiusSq)
		{
			Diver->GhostHauntTime += DeltaTime;
		}
		else
		{
			Diver->GhostHauntTime = FMath::Max(0.f, Diver->GhostHauntTime - DeltaTime * RecoveryRate);
		}

		Diver->SetGhostHauntStage(HauntStageFromTime(Diver->GhostHauntTime));
	}
}

int32 AGhostDirector::HauntStageFromTime(float TimeInRange) const
{
	if (TimeInRange >= Stage3Time) return 3;
	if (TimeInRange >= Stage2Time) return 2;
	if (TimeInRange >= Stage1Time) return 1;
	return 0;
}

void AGhostDirector::DespawnAllGhosts()
{
	for (const TWeakObjectPtr<AGhostCreature>& GhostPtr : ActiveGhosts)
	{
		if (AGhostCreature* Ghost = GhostPtr.Get())
		{
			Ghost->Destroy();
		}
	}
	ActiveGhosts.Reset();
}

void AGhostDirector::PruneGhosts()
{
	ActiveGhosts.RemoveAll([](const TWeakObjectPtr<AGhostCreature>& GhostPtr)
	{
		return !GhostPtr.IsValid();
	});
}
