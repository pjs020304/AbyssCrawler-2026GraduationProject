// Fill out your copyright notice in the Description page of Project Settings.

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

	// 배터리 고갈 시 추진기 자동 정지
	virtual void OnBatteryDepleted() override;

	// 슬롯 해제 시 추진기 정지
	virtual void NotifyUnequipped() override;

	// 장착 중에는 캐릭터가 스쿠터 탑승 포즈로 전환된다
	virtual bool UsesRidePose() const override { return bUseRidePose; }

protected:
	// 탑승 연출 사용 여부. 끄면 기존처럼 손에 든 상태로 표시된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scooter|Ride")
	bool bUseRidePose = true;

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

	void ApplyVisualEffects();
};
