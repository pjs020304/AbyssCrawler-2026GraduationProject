// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h" // 부모 클래스
#include "AbyssFlashLight.generated.h"

class USpotLightComponent;
class AAbyssDiverCharacter;

UCLASS()
class ABYSSCRAWLER_API AAbyssFlashLight : public AAbyssItemBase
{
    GENERATED_BODY()

public:
    AAbyssFlashLight();

protected:
    virtual void BeginPlay() override;

public:
    // [오버라이드] 부모의 UseItem을 재정의 (Toggle)
    virtual void UseItem() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 배터리 고갈 시 강제 꺼짐
    virtual void OnBatteryDepleted() override;

    // 슬롯 해제 시 꺼짐 (부모 NotifyUnequipped + 라이트 끄기)
    virtual void NotifyUnequipped() override;

    // 스팟 라이트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpotLightComponent* SpotLightComp;

    void SetLightEnabled(bool bEnabled);

protected:
    // 손전등을 켤 때 / 끌 때 재생할 효과음.
    // 베이스의 UseSound 대신 이 두 값을 쓴다(손전등은 "사용"이 곧 토글이라 켬/끔을 구분해야 한다).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
    TObjectPtr<USoundBase> LightOnSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
    TObjectPtr<USoundBase> LightOffSound;

private:
    // 직전에 반영한 라이트 상태. 실제로 상태가 "바뀐" 순간에만 소리를 내기 위한 기준값이다.
    bool bLastAppliedLightState = false;

    // 스폰/최초 복제 시점의 첫 반영에서는 소리를 내지 않기 위한 플래그.
    // (뒤늦게 접속한 클라이언트에게 켜져 있는 손전등이 복제될 때 딸깍 소리가 나면 안 된다)
    bool bLightStateInitialized = false;

    UPROPERTY(ReplicatedUsing = OnRep_IsLightOn)
    bool bIsLightOn = false;

    UFUNCTION()
    void OnRep_IsLightOn();

    UFUNCTION(Server, Reliable)
    void Server_SetLightEnabled(bool bNewEnabled);

    // 라이트 상태를 컴포넌트에 반영
    void ApplyLightState();

    // 실제 사용 가능한 상태인지 검사
    bool CanUseFlashLight() const;

    // 강제 꺼짐 (배터리/슬롯 해제 시)
    void ForceTurnOff();
};