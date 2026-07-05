#include "PiranhaSwarmManager.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbyssAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

#include "AbyssDiverCharacter.h"
#include "SVOVolume.h"

#include "EngineUtils.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

APiranhaSwarmManager::APiranhaSwarmManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// The swarm is server-authoritative; its location (the flock centroid) is
	// replicated so clients have an anchor for their cosmetic local flock.
	bReplicates = true;
	SetReplicateMovement(true);

	SwarmRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SwarmRoot"));
	SetRootComponent(SwarmRoot);

	CloudCollision = CreateDefaultSubobject<USphereComponent>(TEXT("CloudCollision"));
	CloudCollision->SetupAttachment(SwarmRoot);
	CloudCollision->InitSphereRadius(SpawnRadius);
	CloudCollision->SetGenerateOverlapEvents(false);

	// One instance per fish. Visual only - the CloudCollision sphere handles hits.
	FishMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FishMesh"));
	FishMesh->SetupAttachment(SwarmRoot);
	FishMesh->SetMobility(EComponentMobility::Movable);
	FishMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FishMesh->SetCollisionProfileName(TEXT("NoCollision"));
	FishMesh->SetGenerateOverlapEvents(false);
	FishMesh->SetCanEverAffectNavigation(false);

	// GAS: swarm-wide health. Minimal replication mode matches the other AI enemies.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UAbyssAttributeSet>(TEXT("AttributeSet"));
}

void APiranhaSwarmManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APiranhaSwarmManager, State);
	DOREPLIFETIME(APiranhaSwarmManager, RepTargetLocation);
	DOREPLIFETIME(APiranhaSwarmManager, RepAliveCount);
}

UAbilitySystemComponent* APiranhaSwarmManager::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APiranhaSwarmManager::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();

	if (RandomSeed > 0)
	{
		RandStream.Initialize(RandomSeed);
	}
	else
	{
		RandStream.GenerateNewSeed();
	}

	// Auto-find an SVO volume for obstacle avoidance if one wasn't assigned.
	if (!SVOVolume)
	{
		for (TActorIterator<ASVOVolume> It(GetWorld()); It; ++It)
		{
			SVOVolume = *It;
			break;
		}
	}

	InitializeBoids();
	UpdateCentroidAndSpread();

	InitializeGAS();

	if (CloudCollision)
	{
		// 작살 등 무기는 그대로 Block으로 맞아 군체에 피해를 입힌다.
		CloudCollision->SetCollisionProfileName(TEXT("Pawn"));

		// 단, 플레이어/AI 폰은 물리적으로 막지 않도록 Pawn 채널만 Ignore.
		// → 플레이어가 군체 구름을 통과하고, AttackRadius 안에 들면 GAS 물기 피해를 받아
		//   "벽에 부딪히는" 느낌 대신 "군체에 덮쳐지는" 느낌이 난다.
		CloudCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	if (HasAuthority())
	{
		RepAliveCount = GetAliveCount();
	}

	InitializeInstances();
	PushToInstances();
}

void APiranhaSwarmManager::InitializeGAS()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Required on both server and clients so the replicated ASC works.
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (!HasAuthority())
	{
		return;
	}

	if (AttributeSet)
	{
		AttributeSet->InitMaxHealth(SwarmMaxHealth);
		AttributeSet->InitHealth(SwarmMaxHealth);
	}

	if (!bGASBound && AttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddUObject(this, &APiranhaSwarmManager::OnSwarmHealthChanged);
		bGASBound = true;
	}

	if (!bAbilitiesInitialized)
	{
		for (const TSubclassOf<UGameplayAbility>& Ability : StartupAbilities)
		{
			if (Ability)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability, 1, INDEX_NONE, this));
			}
		}
		bAbilitiesInitialized = true;
	}
}

void APiranhaSwarmManager::InitializeBoids()
{
	Boids.Reset();
	Boids.Reserve(NumBoids);

	for (int32 i = 0; i < NumBoids; ++i)
	{
		const FVector Offset = RandStream.VRand() * RandStream.FRandRange(0.f, SpawnRadius);
		const FVector Pos = HomeLocation + Offset;
		const FVector Vel = RandStream.VRand() * FMath::Max(MinSpeed, 1.f);
		Boids.Emplace(Pos, Vel);
	}
}

void APiranhaSwarmManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Boids.Num() == 0 || DeltaTime <= 0.f)
	{
		return;
	}

	UpdateCentroidAndSpread();

	// --- Server-authoritative behaviour ---
	if (HasAuthority() && !IsDispersing())
	{
		// Stage 1: count down the lose-interest (investigate) timer.
		if (bInvestigating)
		{
			LoseInterestRemaining -= DeltaTime;
			if (LoseInterestRemaining <= 0.f)
			{
				bInvestigating = false;
			}
		}

		DetectionAccum += DeltaTime;
		if (DetectionAccum >= DetectionInterval)
		{
			DetectionAccum = 0.f;
			UpdateDetection();
		}
		UpdateAttack(DeltaTime);
	}

	// --- Simulation (runs everywhere; authoritative on server, cosmetic on clients) ---
	UpdateAvoidanceSubset();
	SimulateFlocking(DeltaTime);
	UpdateCentroidAndSpread();

	// Keep the actor (and its hit volume) centred on the flock so it replicates.
	if (HasAuthority())
	{
		SetActorLocation(CachedCentroid);
	}
	if (CloudCollision)
	{
		CloudCollision->SetSphereRadius(FMath::Max(CachedSpread, SeparationRadius));
	}

	// --- Visuals ---
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (MeshUpdateRate <= 0.f)
		{
			PushToInstances();
		}
		else
		{
			MeshAccum += DeltaTime;
			const float Interval = 1.f / MeshUpdateRate;
			if (MeshAccum >= Interval)
			{
				MeshAccum -= Interval;
				PushToInstances();
			}
		}
	}
}

void APiranhaSwarmManager::UpdateCentroidAndSpread()
{
	FVector Sum = FVector::ZeroVector;
	int32 Alive = 0;
	for (const FBoid& B : Boids)
	{
		if (B.bAlive)
		{
			Sum += B.Position;
			++Alive;
		}
	}

	if (Alive > 0)
	{
		CachedCentroid = Sum / Alive;

		float MaxDistSq = 0.f;
		for (const FBoid& B : Boids)
		{
			if (B.bAlive)
			{
				MaxDistSq = FMath::Max(MaxDistSq, FVector::DistSquared(B.Position, CachedCentroid));
			}
		}
		CachedSpread = FMath::Sqrt(MaxDistSq);
	}
	else
	{
		CachedCentroid = GetActorLocation();
		CachedSpread = 0.f;
	}
}

FVector APiranhaSwarmManager::SteerTowards(const FBoid& Boid, const FVector& DesiredVelocity) const
{
	const FVector Steer = DesiredVelocity - Boid.Velocity;
	return Steer.GetClampedToMaxSize(MaxSteerForce);
}

void APiranhaSwarmManager::SimulateFlocking(float DeltaTime)
{
	const int32 Count = Boids.Num();
	ScratchAccel.Reset(Count);
	ScratchAccel.SetNumZeroed(Count);

	const bool bDispersing = IsDispersing();
	const bool bChasing = IsChasing();
	const FVector Anchor = GetAnchorLocation();
	const FVector SeekLoc = GetSeekTargetLocation();

	const float NeighborRadiusSq = NeighborRadius * NeighborRadius;
	const float SeparationRadiusSq = SeparationRadius * SeparationRadius;

	for (int32 i = 0; i < Count; ++i)
	{
		const FBoid& Self = Boids[i];
		if (!Self.bAlive)
		{
			continue;
		}

		FVector SeparationDir = FVector::ZeroVector;
		FVector AlignmentSum = FVector::ZeroVector;
		FVector CohesionSum = FVector::ZeroVector;
		int32 NeighborCount = 0;
		int32 SeparationCount = 0;

		for (int32 j = 0; j < Count; ++j)
		{
			if (j == i || !Boids[j].bAlive)
			{
				continue;
			}

			const FVector ToOther = Boids[j].Position - Self.Position;
			const float DistSq = ToOther.SizeSquared();
			if (DistSq > NeighborRadiusSq || DistSq <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			AlignmentSum += Boids[j].Velocity;
			CohesionSum += Boids[j].Position;
			++NeighborCount;

			if (DistSq < SeparationRadiusSq)
			{
				const float Dist = FMath::Sqrt(DistSq);
				SeparationDir += (-ToOther / Dist) / Dist;
				++SeparationCount;
			}
		}

		FVector Accel = FVector::ZeroVector;

		// Separation always applies (keeps fish from overlapping).
		if (SeparationCount > 0 && !SeparationDir.IsNearlyZero())
		{
			const FVector Desired = SeparationDir.GetSafeNormal() * MaxSpeed;
			Accel += SteerTowards(Self, Desired) * SeparationWeight;
		}

		if (bDispersing)
		{
			// Scatter outward from the centre while dying.
			const FVector Outward = (Self.Position - CachedCentroid).GetSafeNormal();
			if (!Outward.IsNearlyZero())
			{
				Accel += Outward * MaxSteerForce;
			}
			ScratchAccel[i] = Accel;
			continue;
		}

		if (NeighborCount > 0)
		{
			const FVector AvgVel = AlignmentSum / NeighborCount;
			if (!AvgVel.IsNearlyZero())
			{
				Accel += SteerTowards(Self, AvgVel.GetSafeNormal() * MaxSpeed) * AlignmentWeight;
			}

			const FVector Center = CohesionSum / NeighborCount;
			const FVector ToCenter = Center - Self.Position;
			if (!ToCenter.IsNearlyZero())
			{
				Accel += SteerTowards(Self, ToCenter.GetSafeNormal() * MaxSpeed) * CohesionWeight;
			}
		}

		// Obstacle avoidance (cached from the SVO probe pass).
		if (!Self.AvoidDir.IsNearlyZero())
		{
			Accel += SteerTowards(Self, Self.AvoidDir.GetSafeNormal() * MaxSpeed) * AvoidanceWeight;
		}

		// Wander: small random jitter so the flock never looks frozen.
		Accel += RandStream.VRand() * (MaxSteerForce * WanderWeight);

		// Seek the target while chasing.
		if (bChasing)
		{
			const FVector ToTarget = SeekLoc - Self.Position;
			if (!ToTarget.IsNearlyZero())
			{
				Accel += SteerTowards(Self, ToTarget.GetSafeNormal() * MaxSpeed) * SeekWeight;
			}
		}

		// Leash: pull back toward the anchor when straying too far.
		const FVector ToAnchor = Anchor - Self.Position;
		if (ToAnchor.Size() > LeashRadius)
		{
			Accel += SteerTowards(Self, ToAnchor.GetSafeNormal() * MaxSpeed) * LeashWeight;
		}

		ScratchAccel[i] = Accel;
	}

	// Integrate.
	const float TopSpeed = bDispersing ? MaxSpeed * DisperseSpeedMultiplier : MaxSpeed;
	const float FloorSpeed = bDispersing ? MinSpeed : MinSpeed;

	for (int32 i = 0; i < Count; ++i)
	{
		FBoid& B = Boids[i];
		if (!B.bAlive)
		{
			continue;
		}

		B.Velocity += ScratchAccel[i] * DeltaTime;

		float Speed = B.Velocity.Size();
		if (Speed < KINDA_SMALL_NUMBER)
		{
			B.Velocity = RandStream.VRand() * FloorSpeed;
		}
		else
		{
			Speed = FMath::Clamp(Speed, FloorSpeed, TopSpeed);
			B.Velocity = B.Velocity.GetSafeNormal() * Speed;
		}

		B.Position += B.Velocity * DeltaTime;
	}
}

void APiranhaSwarmManager::UpdateAvoidanceSubset()
{
	if (!SVOVolume)
	{
		return;
	}

	const int32 N = Boids.Num();
	if (N == 0)
	{
		return;
	}

	const int32 Checks = FMath::Min(AvoidanceChecksPerTick, N);
	for (int32 c = 0; c < Checks; ++c)
	{
		const int32 i = AvoidanceCursor % N;
		++AvoidanceCursor;

		FBoid& B = Boids[i];
		if (!B.bAlive)
		{
			B.AvoidDir = FVector::ZeroVector;
			continue;
		}

		const FVector Fwd = B.Velocity.GetSafeNormal();
		if (Fwd.IsNearlyZero())
		{
			B.AvoidDir = FVector::ZeroVector;
			continue;
		}

		const FVector Ahead = B.Position + Fwd * AvoidanceDistance;
		if (SVOVolume->IsWalkable(Ahead))
		{
			B.AvoidDir = FVector::ZeroVector;
			continue;
		}

		// Obstacle ahead: pick a clear deflection around the velocity axis.
		FVector Right = FVector::CrossProduct(Fwd, FVector::UpVector).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			Right = FVector::CrossProduct(Fwd, FVector::ForwardVector).GetSafeNormal();
		}
		const FVector UpPerp = FVector::CrossProduct(Right, Fwd).GetSafeNormal();

		const FVector Candidates[] = { Right, -Right, UpPerp, -UpPerp };

		FVector Chosen = -Fwd; // fallback: back off
		for (const FVector& Cand : Candidates)
		{
			if (SVOVolume->IsWalkable(B.Position + Cand * AvoidanceDistance))
			{
				Chosen = Cand;
				break;
			}
		}
		B.AvoidDir = Chosen;
	}
}

FVector APiranhaSwarmManager::GetSeekTargetLocation() const
{
	if (HasAuthority() && CurrentTarget.IsValid())
	{
		return CurrentTarget->GetActorLocation();
	}
	return RepTargetLocation;
}

FVector APiranhaSwarmManager::GetAnchorLocation() const
{
	if (IsChasing())
	{
		return GetSeekTargetLocation();
	}
	return HasAuthority() ? HomeLocation : GetActorLocation();
}

void APiranhaSwarmManager::SetSwarmState(EPiranhaSwarmState NewState)
{
	if (State != NewState)
	{
		State = NewState; // replicated; OnRep fires on clients
	}
}

void APiranhaSwarmManager::UpdateDetection()
{
	// --- Stage 3: 영역(Home) 리쉬 ---
	// 둥지에서 너무 멀어지면 타겟/수색을 모두 버리고 둥지로 복귀.
	if (FVector::DistSquared(CachedCentroid, HomeLocation) > FMath::Square(HomeLeashRadius))
	{
		CurrentTarget = nullptr;
		bInvestigating = false;
		RepTargetLocation = HomeLocation;
		SetSwarmState(EPiranhaSwarmState::Patrol);
		return;
	}

	AActor* Target = CurrentTarget.Get();

	// 기존 타겟이 무효하거나 LoseTargetRadius 밖으로 벗어나면 → 흥미 상실(수색) 시작.
	if (Target)
	{
		const float DistSq = FVector::DistSquared(CachedCentroid, Target->GetActorLocation());
		if (!IsValid(Target) || DistSq > FMath::Square(LoseTargetRadius))
		{
			// Stage 1: 마지막 목격 위치를 기억하고 일정 시간 수색.
			if (IsValid(Target))
			{
				LastKnownLocation = Target->GetActorLocation();
			}
			Target = nullptr;
			CurrentTarget = nullptr;
			bInvestigating = true;
			LoseInterestRemaining = LoseInterestTime;
		}
	}

	// Acquire the nearest diver within the detection radius.
	if (!Target)
	{
		float BestDistSq = FMath::Square(DetectRadius);
		AAbyssDiverCharacter* Nearest = nullptr;
		for (TActorIterator<AAbyssDiverCharacter> It(GetWorld()); It; ++It)
		{
			AAbyssDiverCharacter* Diver = *It;
			if (!IsValid(Diver))
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(CachedCentroid, Diver->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Nearest = Diver;
			}
		}
		if (Nearest)
		{
			CurrentTarget = Nearest;
			Target = Nearest;
			bInvestigating = false; // 재포착 → 수색 종료
		}
	}

	if (Target)
	{
		RepTargetLocation = Target->GetActorLocation();
		if (State == EPiranhaSwarmState::Patrol || State == EPiranhaSwarmState::Idle)
		{
			SetSwarmState(EPiranhaSwarmState::Chase);
		}
	}
	else if (bInvestigating)
	{
		// Stage 1: 타겟을 놓쳤지만 아직 수색 중 → 마지막 목격 위치로 계속 이동.
		RepTargetLocation = LastKnownLocation;
		SetSwarmState(EPiranhaSwarmState::Chase);
	}
	else
	{
		SetSwarmState(EPiranhaSwarmState::Patrol);
	}
}

void APiranhaSwarmManager::UpdateAttack(float DeltaTime)
{
	AActor* Target = CurrentTarget.Get();
	if (!IsValid(Target))
	{
		return;
	}

	// 구름이 넓게 퍼져 있으면, 군집 중심이 멀어도 물고기들이 플레이어를 에워싼 상태일 수 있다.
	// AttackRadius와 현재 구름 반경(CachedSpread) 중 큰 값을 기준으로 삼아,
	// 플레이어가 구름 안에 들어오면 곧바로 물기 시작.
	const float AttackDist = FMath::Max(AttackRadius, CachedSpread);
	const float DistSq = FVector::DistSquared(CachedCentroid, Target->GetActorLocation());
	if (DistSq <= FMath::Square(AttackDist))
	{
		SetSwarmState(EPiranhaSwarmState::Attack);
		AttackAccum += DeltaTime;
		if (AttackAccum >= AttackInterval)
		{
			AttackAccum = 0.f;
			PerformBite(Target);
		}
	}
	else if (State == EPiranhaSwarmState::Attack)
	{
		SetSwarmState(EPiranhaSwarmState::Chase);
	}
}

void APiranhaSwarmManager::PerformBite(AActor* Target)
{
	if (!BiteDamageEffect || !AbilitySystemComponent || !IsValid(Target))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(BiteDamageEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}

	// 물린 대상이 플레이어면, 그 플레이어의 클라이언트에서 카메라 흔들림/화면 효과를
	// 재생하도록 알린다(연출은 BP에서 OnSwarmBiteFeedback 구현).
	if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(Target))
	{
		Diver->Client_OnSwarmBite(CachedCentroid);
	}
}

void APiranhaSwarmManager::ApplySwarmDamage(float DamageAmount)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AttributeSet || IsDispersing())
	{
		return;
	}
	AbilitySystemComponent->ApplyModToAttribute(AttributeSet->GetHealthAttribute(), EGameplayModOp::Additive, -DamageAmount);
}

void APiranhaSwarmManager::OnSwarmHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority() || IsDispersing())
	{
		return;
	}

	if (Data.NewValue <= 0.f)
	{
		BeginDisperse();
		return;
	}

	UpdateAliveCountFromHealth();
}

void APiranhaSwarmManager::UpdateAliveCountFromHealth()
{
	if (!AttributeSet)
	{
		return;
	}

	const float MaxH = AttributeSet->GetMaxHealth();
	if (MaxH <= 0.f)
	{
		return;
	}

	const float Ratio = FMath::Clamp(AttributeSet->GetHealth() / MaxH, 0.f, 1.f);
	int32 TargetAlive = FMath::CeilToInt(NumBoids * Ratio);
	TargetAlive = FMath::Clamp(TargetAlive, MinAliveWhileLiving, NumBoids);

	KillBoidsDownTo(TargetAlive);
	RepAliveCount = GetAliveCount();
}

void APiranhaSwarmManager::KillBoidsDownTo(int32 TargetAlive)
{
	int32 Alive = GetAliveCount();
	if (Alive <= TargetAlive)
	{
		return;
	}

	// Cull the fish furthest from the centroid first (they peel off the edges).
	TArray<TPair<float, int32>> AliveByDist;
	AliveByDist.Reserve(Alive);
	for (int32 i = 0; i < Boids.Num(); ++i)
	{
		if (Boids[i].bAlive)
		{
			AliveByDist.Emplace(FVector::DistSquared(Boids[i].Position, CachedCentroid), i);
		}
	}
	AliveByDist.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B)
	{
		return A.Key > B.Key; // furthest first
	});

	int32 ToKill = Alive - TargetAlive;
	for (int32 k = 0; k < ToKill && k < AliveByDist.Num(); ++k)
	{
		Boids[AliveByDist[k].Value].bAlive = false;
	}
}

void APiranhaSwarmManager::BeginDisperse()
{
	SetSwarmState(EPiranhaSwarmState::Dispersing);
	CurrentTarget = nullptr;

	if (CloudCollision)
	{
		CloudCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	GetWorldTimerManager().SetTimer(DisperseTimerHandle, this, &APiranhaSwarmManager::FinishDisperse, DisperseDuration, false);
}

void APiranhaSwarmManager::FinishDisperse()
{
	if (HasAuthority())
	{
		Destroy();
	}
}

void APiranhaSwarmManager::OnRep_State()
{
	// Clients react to state purely through the simulation (chase/disperse use
	// State directly). Hook for VFX/SFX cues can be added here later.
}

void APiranhaSwarmManager::InitializeInstances()
{
	if (!FishMesh)
	{
		return;
	}

	if (FishMeshAsset)
	{
		FishMesh->SetStaticMesh(FishMeshAsset);
		if (FishMaterialOverride)
		{
			FishMesh->SetMaterial(0, FishMaterialOverride);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PiranhaSwarmManager '%s': FishMeshAsset is not set - swarm simulates but renders nothing."),
			*GetName());
	}

	// One instance per boid; transforms are filled in every PushToInstances().
	FishMesh->ClearInstances();
	for (int32 i = 0; i < Boids.Num(); ++i)
	{
		FishMesh->AddInstance(FTransform::Identity, /*bWorldSpace=*/false);
	}
}

void APiranhaSwarmManager::PushToInstances()
{
	if (!FishMesh)
	{
		return;
	}

	const int32 Count = Boids.Num();

	// Keep the instance count in sync with the boid array (handles late changes).
	if (FishMesh->GetInstanceCount() != Count)
	{
		FishMesh->ClearInstances();
		for (int32 i = 0; i < Count; ++i)
		{
			FishMesh->AddInstance(FTransform::Identity, /*bWorldSpace=*/false);
		}
	}

	if (Count == 0)
	{
		return;
	}

	// Server shows every living boid; clients clamp the visible count to the
	// replicated alive count (their local boids never die on their own).
	const int32 Limit = HasAuthority() ? Count : FMath::Clamp(RepAliveCount, 0, Count);

	ScratchTransforms.Reset(Count);
	ScratchTransforms.SetNum(Count);

	const FQuat OffsetQuat = MeshRotationOffset.Quaternion();
	int32 Shown = 0;

	for (int32 i = 0; i < Count; ++i)
	{
		const FBoid& B = Boids[i];
		const bool bVisible = B.bAlive && Shown < Limit;

		if (bVisible)
		{
			const FVector Dir = B.Velocity.GetSafeNormal();
			const FQuat Rot = Dir.IsNearlyZero()
				? OffsetQuat
				: FRotationMatrix::MakeFromX(Dir).ToQuat() * OffsetQuat;
			ScratchTransforms[i] = FTransform(Rot, B.Position, FishScale);
			++Shown;
		}
		else
		{
			// Hidden: collapse to zero scale so the instance disappears.
			ScratchTransforms[i] = FTransform(FQuat::Identity, B.Position, FVector::ZeroVector);
		}
	}

	FishMesh->BatchUpdateInstancesTransforms(0, ScratchTransforms,
		/*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/true, /*bTeleport=*/true);
}

int32 APiranhaSwarmManager::GetAliveCount() const
{
	int32 Count = 0;
	for (const FBoid& B : Boids)
	{
		if (B.bAlive)
		{
			++Count;
		}
	}
	return Count;
}
