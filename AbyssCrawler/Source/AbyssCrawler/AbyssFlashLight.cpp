#include "AbyssFlashLight.h"
#include "AbyssDiverCharacter.h"
#include "Components/SpotLightComponent.h"
#include "Net/UnrealNetwork.h"

AAbyssFlashLight::AAbyssFlashLight()
{
    bReplicates = true;
    SetReplicateMovement(true);

    // 1. 스포트 라이트 생성
    SpotLightComp = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLightComp"));
    SpotLightComp->SetupAttachment(RootComponent);


    //ItemMesh->SetupAttachment(SpotLightComp);
    // 2. 라이트 설정 (이전에 했던 설정 그대로)
    SpotLightComp->Intensity = 15000.0f;
    SpotLightComp->AttenuationRadius = 8000.0f;
    SpotLightComp->OuterConeAngle = 25.0f;
    SpotLightComp->InnerConeAngle = 10.0f;
    SpotLightComp->LightColor = FColor(200, 230, 255);
    SpotLightComp->CastShadows = true;

    // 기본은 꺼둠
    SpotLightComp->SetVisibility(false);
    bIsLightOn = false;

    ItemName = "High-Power Flashlight";
}

void AAbyssFlashLight::BeginPlay()
{
    Super::BeginPlay();
    ApplyLightState();
}

// [사용하기] 클릭하면 불이 켜졌다 꺼졌다 함
void AAbyssFlashLight::UseItem()
{
    Super::UseItem(); // 부모 로그 출력

    // 손전등을 실제 사용할 수 있는 상태가 아니면 무시
    if (!CanUseFlashLight())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FlashLight] Cannot use flashlight now."));
        return;
    }

    const bool bNewLightState = !bIsLightOn;

    //bIsLightOn = !bIsLightOn;
    //SpotLightComp->SetVisibility(bIsLightOn);

    if (HasAuthority())
    {
        bIsLightOn = bNewLightState;
        ApplyLightState();
    }
    else
    {
        Server_SetLightEnabled(bNewLightState);
    }


    UE_LOG(LogTemp, Warning, TEXT("[FlashLight] UseItem called, LightOn=%s"), bIsLightOn ? TEXT("true") : TEXT("false"));
    
    // 딸깍 소리 추가 가능
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
    bIsLightOn = false;
    ApplyLightState();
}

void AAbyssFlashLight::NotifyUnequipped()
{
    // 슬롯 전환 등으로 손에서 내려가면 꺼두는 기능
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
}

