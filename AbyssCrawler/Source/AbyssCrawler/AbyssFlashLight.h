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

private:
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