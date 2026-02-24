#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h" // 부모 헤더 포함
#include "AbyssFlashLight.generated.h"

class USpotLightComponent;


UCLASS()
class ABYSSCRAWLER_API AAbyssFlashLight : public AAbyssItemBase
{
    GENERATED_BODY()

public:
    AAbyssFlashLight();

protected:
    virtual void BeginPlay() override;

public:
    // [핵심] 부모의 UseItem을 내 입맛대로 변경 (Override)
    virtual void UseItem() override;

    // 손전등 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpotLightComponent* SpotLightComp;

private:
    bool bIsLightOn;
};