#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "HarpoonProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class ABYSSCRAWLER_API AHarpoonProjectile : public AActor
{
	GENERATED_BODY()

public:
	AHarpoonProjectile();

	// 발사자(및 총 액터)와의 자체 충돌 방지.
	// 총구가 캐릭터 몸에 붙어 있어 스폰 직후 자기 자신에 걸릴 수 있으므로 발사 시 호출한다.
	void IgnoreShooter(AActor* ShooterActor);

protected:
	virtual void BeginPlay() override;

	// 충돌을 감지할 구체 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionComp;

	// 작살의 외형
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	// 날아가는 물리 이동을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovement;

	// 상어에게 보낼 GAS 이벤트 태그 (예: "Event.Attack.Harpoon")
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag HarpoonHitEventTag;

	// 충돌 시 호출될 함수
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};