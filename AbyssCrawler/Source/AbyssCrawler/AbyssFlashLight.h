#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h" // 부모 헤더 포함
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
    // [핵심] 부모의 UseItem을 내 입맛대로 변경 (Override)
    virtual void UseItem() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 손전등 컴포넌트
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

    // 손전등 상태를 실제 컴포넌트에 반영
    void ApplyLightState();

    // 이 손전등이 지금 사용 가능한 상태인지 검사
    bool CanUseFlashLight() const;

    // 손전등을 강제로 끔 (드롭/숨김 시 사용)
    void ForceTurnOff();

public:
    // 필요 시 캐릭터 쪽에서 호출 가능하도록 public
    void NotifyUnequipped();

};