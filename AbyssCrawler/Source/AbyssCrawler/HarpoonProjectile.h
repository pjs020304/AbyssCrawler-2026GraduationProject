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