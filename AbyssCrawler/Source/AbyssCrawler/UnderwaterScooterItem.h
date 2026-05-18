#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h"
#include "Camera/CameraShakeBase.h"
#include "UnderwaterScooterItem.generated.h"

class UNiagaraComponent;

UCLASS()
class ABYSSCRAWLER_API AUnderwaterScooterItem : public AAbyssItemBase
{
	GENERATED_BODY()
	
public:
	AUnderwaterScooterItem();

	virtual void UseItem() override;
	virtual void EndUseItem() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Scooter")
	float PropulsionForce = 500000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Scooter")
	TSubclassOf<UCameraShakeBase> ScooterShakeClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* BubbleParticleComp;

	UPROPERTY(ReplicatedUsing = OnRep_IsActive)
	bool bIsActive = false;

	UFUNCTION()
	void OnRep_IsActive();

	// For battery consumption
	FTimerHandle BatteryDrainTimer;

	UFUNCTION()
	void DrainBattery();

	void ApplyVisualEffects();
};
