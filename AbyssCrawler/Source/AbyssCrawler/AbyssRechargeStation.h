#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssRechargeStation.generated.h"

class UWidgetComponent;

UCLASS()
class ABYSSCRAWLER_API AAbyssRechargeStation : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()

public:
	AAbyssRechargeStation();

	// E키 상호작용
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	// "E 눌러서 충전" 등을 띄울 UI
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UWidgetComponent* InteractWidget;

	// 플레이어에게 적용할 충전 이펙트 (산소 100% 회복 또는 배터리 100% 회복)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Station Config")
	TSubclassOf<class UGameplayEffect> RechargeEffectClass;

	// 한 번 누르면 몇 초 뒤에 다시 쓸 수 있는지 (쿨타임)
	UPROPERTY(EditDefaultsOnly, Category = "Station Config")
	float RechargeCooldown;

	bool bIsOnCooldown;
	FTimerHandle CooldownTimerHandle;
	void ResetCooldown();
};