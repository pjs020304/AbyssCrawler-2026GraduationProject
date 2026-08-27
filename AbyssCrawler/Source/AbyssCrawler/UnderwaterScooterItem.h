// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h"
#include "Camera/CameraShakeBase.h"
#include "UnderwaterScooterItem.generated.h"

class UAudioComponent;
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

	// 추진기가 도는 동안 계속 흐르는 엔진음. 반복 재생될 사운드를 지정할 것(Looping 체크된 SoundWave/SoundCue).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> EngineLoopSound;

	// 켜는 순간 / 끄는 순간 한 번씩 울리는 효과음.
	// 베이스의 UseSound 대신 이 두 값을 쓴다(추진기는 "사용"이 곧 on/off 토글이다).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> StartSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> StopSound;

	// 엔진음 루프 재생용. 아이템이 손에 부착되어 있으므로 소리도 사용자를 따라다닌다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* EngineAudioComp;

	UPROPERTY(ReplicatedUsing = OnRep_IsActive)
	bool bIsActive = false;

	UFUNCTION()
	void OnRep_IsActive();

	void ApplyVisualEffects();

private:
	// 직전에 반영한 작동 상태. 실제로 바뀐 순간에만 시작/정지 효과음을 내기 위한 기준값이다.
	bool bLastAppliedActiveState = false;

	// 스폰/최초 복제 시점의 첫 반영에서는 일회성 효과음을 내지 않는다.
	// (이미 작동 중인 추진기가 뒤늦게 복제될 때 시동음이 울리면 안 된다. 루프음은 그때도 켜져야 하므로 별개로 처리한다)
	bool bActiveStateInitialized = false;
};
