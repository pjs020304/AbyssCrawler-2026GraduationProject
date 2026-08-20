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

void APiranhaSwarmManager::ValidateFishMesh() const
{
	if (!FishMeshAsset)
	{
		return;
	}

	const FBoxSphereBounds Bounds = FishMeshAsset->GetBounds();
	const FVector Ext = Bounds.BoxExtent;
	const float SideSpan = FMath::Max(Ext.Y, Ext.Z);

	// 물고기 한 마리는 진행축(X)으로 길고 옆으로 얇다. 옆폭이 길이에 맞먹으면
	// 여러 마리가 뭉쳐 있는 덩어리일 가능성이 높다. 어디까지나 휴리스틱이라
	// 막지 않고 경고만 남긴다.
	const bool bLooksLikeCluster = (Ext.X > KINDA_SMALL_NUMBER) && (SideSpan > Ext.X * 0.6f);

	if (bLooksLikeCluster && FishPerMeshInstance <= 1)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PiranhaSwarmManager '%s': FishMeshAsset '%s' 의 형태가 물고기 한 마리로 보이지 않습니다 ")
			TEXT("(Extent X=%.1f Y=%.1f Z=%.1f). 여러 마리가 한 덩어리로 들어 있으면 그 덩어리가 통째로 ")
			TEXT("움직여서 '판이 흔들리는' 것처럼 보입니다. 메시를 1마리로 잘라 쓰거나, 임시로 ")
			TEXT("FishPerMeshInstance 를 실제 마릿수로 설정하세요."),
			*GetName(), *FishMeshAsset->GetName(), Ext.X, Ext.Y, Ext.Z);
	}

	// 최종 렌더 크기를 남겨 FishScale 튜닝에 쓸 수 있게 한다 (피라냐 기준 약 30cm).
	const float RenderedLength = Ext.X * 2.f * FMath::Abs(FishScale.X);
	UE_LOG(LogTemp, Log,
		TEXT("PiranhaSwarmManager '%s': 물고기 1마리 렌더 길이 약 %.1f cm (FishScale=%.3f), ")
		TEXT("인스턴스 %d개 x %d마리 = 총 %d마리"),
		*GetName(), RenderedLength, FishScale.X,
		GetSimBoidCount(), FMath::Max(1, FishPerMeshInstance),
		GetSimBoidCount() * FMath::Max(1, FishPerMeshInstance));
}

int32 APiranhaSwarmManager::GetSimBoidCount() const
{
	// NumBoids는 "화면에 보이는 총 마릿수"라는 뜻을 유지한다. 메시 한 덩어리에
	// 여러 마리가 들어 있으면 그만큼 인스턴스를 적게 만들어야 총 마릿수가 맞는다.
	const int32 PerInstance = FMath::Max(1, FishPerMeshInstance);
	return FMath::Max(1, FMath::DivideAndRoundUp(NumBoids, PerInstance));
}

void APiranhaSwarmManager::InitializeBoids()
{
	const int32 Count = GetSimBoidCount();

	Boids.Reset();
	Boids.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Offset = RandStream.VRand() * RandStream.FRandRange(0.f, SpawnRadius);
		const FVector Pos = HomeLocation + Offset;
		const FVector Vel = RandStream.VRand() * FMath::Max(MinSpeed, 1.f);
		FBoid& B = Boids.Emplace_GetRef(Pos, Vel);
		// Stable animation phase so every fish beats its tail on its own cycle.
		B.Phase = RandStream.FRand();
		// 첫 프레임에 기본 방향에서 홱 돌아가지 않도록 진행 방향으로 초기화.
		B.Forward = Vel.GetSafeNormal();
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

		// 렌더링용 방향을 속도 방향으로 서서히 추종시킨다.
		// 지수 감쇠라 프레임레이트가 달라져도 회전 체감이 같다.
		const FVector DesiredFwd = B.Velocity.GetSafeNormal();
		if (!DesiredFwd.IsNearlyZero())
		{
			if (TurnSmoothingRate > 0.f)
			{
				const float Alpha = 1.f - FMath::Exp(-TurnSmoothingRate * DeltaTime);
				const FVector Blended = FMath::Lerp(B.Forward, DesiredFwd, Alpha);
				B.Forward = Blended.IsNearlyZero() ? DesiredFwd : Blended.GetSafeNormal();
			}
			else
			{
				B.Forward = DesiredFwd;
			}
		}
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

	// 체력 비율 → 살아있는 인스턴스 수. 기준은 NumBoids가 아니라 실제 인스턴스 수여야
	// 한다(메시 한 덩어리에 여러 마리면 둘이 다르다).
	const int32 SimCount = GetSimBoidCount();
	const float Ratio = FMath::Clamp(AttributeSet->GetHealth() / MaxH, 0.f, 1.f);
	int32 TargetAlive = FMath::CeilToInt(SimCount * Ratio);
	TargetAlive = FMath::Clamp(TargetAlive, FMath::Min(MinAliveWhileLiving, SimCount), SimCount);

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

	ValidateFishMesh();

	// One instance per boid; transforms are filled in every PushToInstances().
	// Custom data floats must be declared before AddInstance so each new instance
	// grows the custom data buffer with it.
	FishMesh->ClearInstances();
	FishMesh->SetNumCustomDataFloats(SwarmCustomDataFloats);
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
		FishMesh->SetNumCustomDataFloats(SwarmCustomDataFloats);
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

	// The fish mesh is a static mesh with no skeletal animation, so the swim
	// motion is produced in the material from these per-instance values.
	// Skipped (rather than asserting) if the count was overridden on the component.
	const bool bWriteCustomData = (FishMesh->NumCustomDataFloats == SwarmCustomDataFloats);
	if (bWriteCustomData)
	{
		ScratchCustomData.Reset(Count * SwarmCustomDataFloats);
		ScratchCustomData.SetNum(Count * SwarmCustomDataFloats);
	}

	const FQuat OffsetQuat = MeshRotationOffset.Quaternion();
	const float SpeedRange = FMath::Max(MaxSpeed - MinSpeed, 1.f);
	int32 Shown = 0;

	for (int32 i = 0; i < Count; ++i)
	{
		const FBoid& B = Boids[i];
		const bool bVisible = B.bAlive && Shown < Limit;

		if (bVisible)
		{
			// 스무딩된 방향을 쓴다. 속도 방향을 그대로 쓰면 매 프레임 미세하게 튄다.
			FVector Dir = B.Forward;
			if (Dir.IsNearlyZero())
			{
				Dir = B.Velocity.GetSafeNormal();
			}

			FQuat Rot = OffsetQuat;
			if (!Dir.IsNearlyZero())
			{
				// MakeFromX는 X축만 맞추고 나머지 두 축은 임의로 잡는다. 그래서 진행 방향이
				// 조금만 바뀌어도 롤이 확 뒤집히고, 이게 "판이 뒤집히며 파닥거리는" 주범이다.
				// 월드 Z를 위쪽으로 고정해서 롤을 안정시킨다.
				FVector Up = FVector::UpVector;
				if (FMath::Abs(FVector::DotProduct(Dir, Up)) > 0.99f)
				{
					// 거의 수직으로 오르내릴 때는 Z를 up으로 못 쓰므로 다른 축으로 폴백.
					Up = FVector::ForwardVector;
				}
				Rot = FRotationMatrix::MakeFromXZ(Dir, Up).ToQuat() * OffsetQuat;
			}

			ScratchTransforms[i] = FTransform(Rot, B.Position, FishScale);
			++Shown;
		}
		else
		{
			// Hidden: collapse to zero scale so the instance disappears.
			ScratchTransforms[i] = FTransform(FQuat::Identity, B.Position, FVector::ZeroVector);
		}

		if (bWriteCustomData)
		{
			const int32 Base = i * SwarmCustomDataFloats;
			ScratchCustomData[Base + 0] = B.Phase;
			ScratchCustomData[Base + 1] = bVisible
				? FMath::Clamp((B.Velocity.Size() - MinSpeed) / SpeedRange, 0.f, 1.f)
				: 0.f;
			ScratchCustomData[Base + 2] = bVisible ? 1.f : 0.f;
		}
	}

	// Write custom data without dirtying, then let the transform batch mark the
	// render state dirty once for both payloads.
	if (bWriteCustomData)
	{
		FishMesh->SetCustomData(0, Count - 1, ScratchCustomData, /*bMarkRenderStateDirty=*/false);
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
