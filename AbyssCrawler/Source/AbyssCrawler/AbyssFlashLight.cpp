#include "AbyssFlashLight.h"
#include "Components/SpotLightComponent.h"

AAbyssFlashLight::AAbyssFlashLight()
{
    // 1. 스포트 라이트 생성
    SpotLightComp = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLightComp"));
    RootComponent = SpotLightComp; // 아이템의 뿌리가 됨

    // 2. 라이트 설정 (이전에 했던 설정 그대로)
    SpotLightComp->Intensity = 5000.0f;
    SpotLightComp->AttenuationRadius = 2000.0f;
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
}

// [사용하기] 클릭하면 불이 켜졌다 꺼졌다 함
void AAbyssFlashLight::UseItem()
{
    Super::UseItem(); // 부모 로그 출력

    bIsLightOn = !bIsLightOn;
    SpotLightComp->SetVisibility(bIsLightOn);

    // 딸깍 소리 추가 가능
}