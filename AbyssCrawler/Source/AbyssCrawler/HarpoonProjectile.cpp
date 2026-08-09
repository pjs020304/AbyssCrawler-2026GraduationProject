#include "HarpoonProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

AHarpoonProjectile::AHarpoonProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	// 1. 충돌체 설정
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AHarpoonProjectile::OnHit);
	RootComponent = CollisionComp;

	// 2. 메쉬 설정
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 발사체 이동 설정 (수중이므로 중력 영향을 줄이거나 없앱니다)
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.0f; // 작살의 발사 속도
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->ProjectileGravityScale = 0.1f; // 수중 저항/부력 느낌
}

void AHarpoonProjectile::BeginPlay()
{
	Super::BeginPlay();
	// 5초 뒤에 어딘가 안 맞았어도 자동 소멸 (메모리 누수 방지)
	SetLifeSpan(5.0f);
}

void AHarpoonProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	// 자기 자신을 쏜 사람(플레이어)은 무시
	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherActor != GetInstigator()))
	{
		// 4. 충돌한 대상(상어)에게 GAS 이벤트(편지) 발송!
		FGameplayEventData Payload;
		Payload.EventTag = HarpoonHitEventTag;
		Payload.Instigator = GetInstigator(); // 플레이어
		Payload.Target = OtherActor;          // 맞은 대상 (상어)

		// 타겟에게 "너 작살 맞았어!" 라고 알림
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OtherActor, HarpoonHitEventTag, Payload);
		UE_LOG(LogTemp, Warning, TEXT("Harpoon Hit Payload"));
		// 작살 파괴
		Destroy();

		return;
	}
}