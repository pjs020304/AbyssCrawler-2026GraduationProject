// Fill out your copyright notice in the Description page of Project Settings.

#include "AbyssFlashLight.h"
#include "AbyssDiverCharacter.h"
#include "Components/SpotLightComponent.h"
#include "Net/UnrealNetwork.h"

AAbyssFlashLight::AAbyssFlashLight()
{
    bReplicates = true;
    SetReplicateMovement(true);

    // 1. 스팟라이트 컴포넌트 생성
    SpotLightComp = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLightComp"));
    SpotLightComp->SetupAttachment(RootComponent);

    // 2. 라이트 설정
    SpotLightComp->Intensity = 15000.0f;
    SpotLightComp->AttenuationRadius = 8000.0f;
    SpotLightComp->OuterConeAngle = 25.0f;
    SpotLightComp->InnerConeAngle = 10.0f;
    SpotLightComp->LightColor = FColor(200, 230, 255);
    SpotLightComp->CastShadows = true;

    // 기본적으로 꺼둠
    SpotLightComp->SetVisibility(false);
    bIsLightOn = false;

    ItemName = TEXT("High-Power Flashlight");
}

void AAbyssFlashLight::BeginPlay()
{
    Super::BeginPlay();
    ApplyLightState();
}

// [사용하기] 클릭하면 불이 켜졌다 꺼졌다
void AAbyssFlashLight::UseItem()
{
    // 실제로 사용할 수 있는 상태가 아니면 무시
    if (!CanUseFlashLight())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FlashLight] Cannot use flashlight now."));
        return;
    }

    // 주의: 일회성 배터리 소모(Super::UseItem())를 호출하지 않음.
    // 손전등은 지속 소모(PassiveDrain)만 사용.

    const bool bNewLightState = !bIsLightOn;

    if (HasAuthority())
    {
        bIsLightOn = bNewLightState;
        ApplyLightState();

        // 켜질 때 지속 소모 시작, 꺼질 때 정지
        if (bIsLightOn)
        {
            StartPassiveDrain();
        }
        else
        {
            StopPassiveDrain();
        }
    }
    else
    {
        Server_SetLightEnabled(bNewLightState);
    }

    UE_LOG(LogTemp, Warning, TEXT("[FlashLight] UseItem called, LightOn=%s"), bIsLightOn ? TEXT("true") : TEXT("false"));
}

void AAbyssFlashLight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssFlashLight, bIsLightOn);
}

void AAbyssFlashLight::SetLightEnabled(bool bEnabled)
{
    if (HasAuthority())
    {
        bIsLightOn = bEnabled;
        ApplyLightState();

        if (bIsLightOn)
        {
            StartPassiveDrain();
        }
        else
        {
            StopPassiveDrain();
        }
    }
    else
    {
        Server_SetLightEnabled(bEnabled);
    }
}

void AAbyssFlashLight::OnRep_IsLightOn()
{
    ApplyLightState();
}

void AAbyssFlashLight::ApplyLightState()
{
    if (!SpotLightComp)
    {
        return;
    }

    SpotLightComp->SetVisibility(bIsLightOn);

    UE_LOG(LogTemp, Warning, TEXT("[FlashLight] ApplyLightState, LightOn=%s"),
        bIsLightOn ? TEXT("true") : TEXT("false"));
}

bool AAbyssFlashLight::CanUseFlashLight() const
{
    return bPickedUp;
}

void AAbyssFlashLight::ForceTurnOff()
{
    // 라이트 끄기
    bIsLightOn = false;
    ApplyLightState();

    // 지속 소모도 정지
    StopPassiveDrain();
}

void AAbyssFlashLight::NotifyUnequipped()
{
    // 슬롯에서 해제되면 강제로 꺼짐 + 지속 소모 정지
    ForceTurnOff();

    // 부모의 NotifyUnequipped도 호출 (StopPassiveDrain은 ForceTurnOff에서 이미 처리하지만 안전하게 호출)
    Super::NotifyUnequipped();
}

void AAbyssFlashLight::OnBatteryDepleted()
{
    UE_LOG(LogTemp, Warning, TEXT("[FlashLight] Battery depleted! Force turning off."));

    // 서버에서 강제 꺼짐 (bIsLightOn은 Replicated이므로 클라에도 자동 전파)
    ForceTurnOff();
}

void AAbyssFlashLight::Server_SetLightEnabled_Implementation(bool bNewEnabled)
{
    // 서버에서 최종 검증
    if (!bPickedUp)
    {
        ForceTurnOff();
        return;
    }

    bIsLightOn = bNewEnabled;
    ApplyLightState();

    // 켜질 때 지속 소모 시작, 꺼질 때 정지
    if (bIsLightOn)
    {
        StartPassiveDrain();
    }
    else
    {
        StopPassiveDrain();
    }
}
