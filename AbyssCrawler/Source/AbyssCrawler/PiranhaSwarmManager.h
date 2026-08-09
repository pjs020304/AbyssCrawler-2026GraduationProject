#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "PiranhaSwarmManager.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class USphereComponent;
class UAbilitySystemComponent;
class UAbyssAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class ASVOVolume;
struct FOnAttributeChangeData;

// Lightweight per-fish simulation data.
// Plain struct (not a USTRUCT): runtime sim state, never exposed to BP and never
// reflected/replicated directly. Kept tight for cache-friendly loops.
struct FBoid
{
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	// Cached obstacle-avoidance steering direction (unit), refreshed round-robin.
	FVector AvoidDir = FVector::ZeroVector;
	// Constant per-fish animation phase (0..1), handed to the swim material as
	// per-instance custom data so the flock doesn't beat its tail in lockstep.
	float Phase = 0.f;
	bool bAlive = true;

	FBoid() {}
	FBoid(const FVector& InPos, const FVector& InVel)
		: Position(InPos), Velocity(InVel) {}
};

// High-level behaviour state of the whole swarm.
UENUM(BlueprintType)
enum class EPiranhaSwarmState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Patrol		UMETA(DisplayName = "Patrol"),
	Chase		UMETA(DisplayName = "Chase"),
	Attack		UMETA(DisplayName = "Attack"),
	Dispersing	UMETA(DisplayName = "Dispersing")
};

/**
 * Manager actor that simulates a flock of piranhas (boids) and renders them
 * through a single Instanced Static Mesh component (one instance per fish).
 * One actor drives hundreds of fish; the CPU sim writes instance transforms.
 *
 *  Phase 0/1: spawn + Reynolds flocking (separation / alignment / cohesion) + wander + leash.
 *  Phase 2  : SVO obstacle avoidance.
 *  Phase 3  : player detection -> chase -> periodic bite damage (GAS).
 *  Phase 4  : swarm-wide HP (ASC + AttributeSet). Damage shrinks the boid count;
 *             at 0 HP the flock scatters and the actor is destroyed.
 *  Phase 5  : server authoritative simulation + minimal replication; clients run a
 *             local cosmetic flock seeded from the replicated centroid/target/count.
 */
UCLASS()
class ABYSSCRAWLER_API APiranhaSwarmManager : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APiranhaSwarmManager();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// Test/utility hook: apply raw damage to the swarm (server only).
	UFUNCTION(BlueprintCallable, Category = "Swarm|Combat")
	void ApplySwarmDamage(float DamageAmount);

protected:
	virtual void BeginPlay() override;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Swarm")
	USceneComponent* SwarmRoot;

	// One instance per fish. The CPU flock writes a transform per instance each
	// update; the component is purely visual (no collision).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Swarm")
	UInstancedStaticMeshComponent* FishMesh;

	// Hit volume around the flock centroid so weapons (harpoon) can damage the swarm.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Swarm")
	USphereComponent* CloudCollision;

	// --- GAS (Phase 4) ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Swarm|GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Swarm|GAS")
	UAbyssAttributeSet* AttributeSet;

	// Abilities granted on spawn (e.g. the shared "take harpoon damage" ability).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Swarm|GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	// Total hit points of the whole swarm.
	UPROPERTY(EditAnywhere, Category = "Swarm|GAS")
	float SwarmMaxHealth = 300.f;

	// Minimum number of fish kept alive while the swarm still has HP (> 0).
	UPROPERTY(EditAnywhere, Category = "Swarm|GAS")
	int32 MinAliveWhileLiving = 1;

	// --- Visual (Instanced Static Mesh) ---

	// The fish mesh drawn once per boid. Must be a *static* mesh: instanced
	// rendering is what lets one actor draw the whole flock in a single pass.
	// The imported Swarm asset is a skeletal mesh with no animation sequence, so
	// convert it once via the Skeletal Mesh Editor toolbar > "Make Static Mesh"
	// and assign the result here. Swim motion comes from the material below.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swarm|Visual")
	UStaticMesh* FishMeshAsset;

	// Material applied to element 0. Expected to be a World Position Offset
	// "swim wiggle" material reading the per-instance custom data this actor
	// writes every mesh update (see PushToInstances):
	//   Custom Data 0 = phase offset (0..1, constant per fish)
	//   Custom Data 1 = speed alpha  (0..1, MinSpeed..MaxSpeed)
	//   Custom Data 2 = visibility   (1 = drawn, 0 = culled)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swarm|Visual")
	UMaterialInterface* FishMaterialOverride;

	// Per-instance scale of the fish mesh. The converted Swarm mesh comes in at
	// roughly 1 m, so 0.3 lands on the ~0.3 m piranha size from the design notes.
	// Editable per placed swarm; the ratio lock keeps the three axes uniform.
	UPROPERTY(EditAnywhere, Category = "Swarm|Visual", meta = (AllowPreserveRatio, ClampMin = "0.001", UIMin = "0.01", UIMax = "2.0"))
	FVector FishScale = FVector(0.3f, 0.3f, 0.3f);

	// Rotation offset so the mesh's forward axis lines up with travel direction.
	// Default assumes the mesh faces +X (Unreal forward). Adjust if it faces another axis.
	UPROPERTY(EditAnywhere, Category = "Swarm|Visual")
	FRotator MeshRotationOffset = FRotator::ZeroRotator;

	// How often (Hz) instance transforms are refreshed. 0 = every frame.
	UPROPERTY(EditAnywhere, Category = "Swarm|Visual")
	float MeshUpdateRate = 30.f;

	// --- Setup ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swarm|Setup", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 NumBoids = 150;

	UPROPERTY(EditAnywhere, Category = "Swarm|Setup")
	float SpawnRadius = 300.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Setup")
	int32 RandomSeed = 0;

	// --- Flocking ---

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float NeighborRadius = 250.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float SeparationRadius = 90.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float SeparationWeight = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float AlignmentWeight = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float CohesionWeight = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float WanderWeight = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float LeashRadius = 700.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Flocking")
	float LeashWeight = 1.2f;

	// --- Movement limits ---

	UPROPERTY(EditAnywhere, Category = "Swarm|Movement")
	float MaxSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Movement")
	float MinSpeed = 120.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Movement")
	float MaxSteerForce = 900.f;

	// --- Obstacle Avoidance (Phase 2) ---

	// SVO volume used for collision probes. Auto-found in BeginPlay if left null.
	UPROPERTY(EditAnywhere, Category = "Swarm|Avoidance")
	ASVOVolume* SVOVolume;

	// How far ahead a boid probes for obstacles.
	UPROPERTY(EditAnywhere, Category = "Swarm|Avoidance")
	float AvoidanceDistance = 220.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Avoidance")
	float AvoidanceWeight = 2.5f;

	// Number of boids re-probed against the SVO each tick (round-robin, amortized).
	UPROPERTY(EditAnywhere, Category = "Swarm|Avoidance", meta = (ClampMin = "1"))
	int32 AvoidanceChecksPerTick = 40;

	// --- Combat (Phase 3) ---

	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float DetectRadius = 1200.f;

	// Target is dropped once the centroid is further than this.
	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float LoseTargetRadius = 2200.f;

	// Stage 1: after losing the target, investigate its last known location for
	// this long before fully giving up and returning to patrol.
	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float LoseInterestTime = 8.f;

	// Stage 3: if the swarm centroid strays this far from HomeLocation, abandon
	// the chase and return home regardless of target distance.
	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float HomeLeashRadius = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float SeekWeight = 1.5f;

	// Centroid-to-target distance at which the swarm starts biting.
	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float AttackRadius = 300.f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float AttackInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Combat")
	float DetectionInterval = 0.25f;

	// Damage effect applied to a bitten player. Assign GE_PiranhaBite in editor.
	UPROPERTY(EditDefaultsOnly, Category = "Swarm|Combat")
	TSubclassOf<UGameplayEffect> BiteDamageEffect;

	// --- Death / Disperse (Phase 4) ---

	UPROPERTY(EditAnywhere, Category = "Swarm|Death")
	float DisperseDuration = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Swarm|Death")
	float DisperseSpeedMultiplier = 1.8f;

	// --- Replicated state (Phase 5) ---

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Swarm|State")
	EPiranhaSwarmState State = EPiranhaSwarmState::Patrol;

	UPROPERTY(Replicated)
	FVector_NetQuantize RepTargetLocation = FVector::ZeroVector;

	// Authoritative alive-fish count; clients clamp their visible flock to this.
	UPROPERTY(Replicated)
	int32 RepAliveCount = 0;

private:
	// Simulation data (authoritative on server, cosmetic on clients).
	TArray<FBoid> Boids;

	FVector HomeLocation = FVector::ZeroVector; // patrol anchor (server)
	FVector CachedCentroid = FVector::ZeroVector;
	float CachedSpread = 0.f;

	float MeshAccum = 0.f;
	float DetectionAccum = 0.f;
	float AttackAccum = 0.f;
	int32 AvoidanceCursor = 0;

	TWeakObjectPtr<AActor> CurrentTarget;

	// Stage 1 (lose-interest) state.
	FVector LastKnownLocation = FVector::ZeroVector;
	bool bInvestigating = false;
	float LoseInterestRemaining = 0.f;

	// Number of per-instance custom data floats written for the swim material.
	static constexpr int32 SwarmCustomDataFloats = 3;

	// Reused each frame to avoid per-tick heap allocations.
	TArray<FTransform> ScratchTransforms;
	TArray<FVector> ScratchAccel;
	TArray<float> ScratchCustomData;

	FRandomStream RandStream;

	FTimerHandle DisperseTimerHandle;

	// --- Core sim ---
	void InitializeBoids();
	void SimulateFlocking(float DeltaTime);
	void UpdateCentroidAndSpread();
	FVector SteerTowards(const FBoid& Boid, const FVector& DesiredVelocity) const;

	// --- Rendering (Instanced Static Mesh) ---
	void InitializeInstances();
	void PushToInstances();

	// --- Avoidance ---
	void UpdateAvoidanceSubset();

	// --- Combat / state (server) ---
	void UpdateDetection();
	void UpdateAttack(float DeltaTime);
	void PerformBite(AActor* Target);

	// --- GAS / health (Phase 4) ---
	void InitializeGAS();
	void OnSwarmHealthChanged(const FOnAttributeChangeData& Data);
	void UpdateAliveCountFromHealth();
	void KillBoidsDownTo(int32 TargetAlive);
	void BeginDisperse();
	void FinishDisperse();

	UFUNCTION()
	void OnRep_State();

	// --- Helpers ---
	int32 GetAliveCount() const;
	bool IsChasing() const { return State == EPiranhaSwarmState::Chase || State == EPiranhaSwarmState::Attack; }
	bool IsDispersing() const { return State == EPiranhaSwarmState::Dispersing; }
	FVector GetSeekTargetLocation() const;
	FVector GetAnchorLocation() const;
	void SetSwarmState(EPiranhaSwarmState NewState);

	bool bAbilitiesInitialized = false;
	bool bGASBound = false;
};
