#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h"
#include "HarpoonGunItem.generated.h"

class AHarpoonProjectile;

UCLASS()
class ABYSSCRAWLER_API AHarpoonGunItem : public AAbyssItemBase
{
	GENERATED_BODY()

public:
	// 좌클릭 등 아이템 사용 시 호출되는 함수 오버라이드
	virtual void UseItem() override;

	// 발사 쿨타임 판정
	virtual bool CanUseItem() const override;
	virtual void NotifyUseAttempted() override;

	// 남은 쿨타임(초). 0이면 발사 가능. UI 표시용.
	UFUNCTION(BlueprintPure, Category = "Weapon|Cooldown")
	float GetFireCooldownRemaining() const;

protected:
	// --- 발사 쿨타임 ---
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Cooldown", meta = (ClampMin = "0.0"))
	float FireCooldown = 2.0f;

	// 서버 권위 발사 시각. 실제로 발사가 성사된 순간에만 기록된다.
	float LastFireTime = -FLT_MAX;

	// 클라이언트 예측용 발사 시각. 서버 왕복을 기다리지 않고 연출/입력을 막기 위한 것으로,
	// 최종 판정은 어디까지나 서버의 LastFireTime이 한다.
	float LastLocalFireTime = -FLT_MAX;

	// 에디터에서 할당할 발사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AHarpoonProjectile> ProjectileClass;

	// --- 총구(Muzzle) ---
	// 발사 위치로 쓸 ItemMesh의 소켓 이름. 스태틱 메시 에디터에서 추가한다.
	// 소켓이 없으면 아래 MuzzleOffset으로 대체된다.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Muzzle")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 소켓이 없을 때 사용할 총구 위치 (ItemMesh 로컬 기준)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Muzzle")
	FVector MuzzleOffset = FVector::ZeroVector;

	// 조준점을 찾기 위한 시선 트레이스 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Muzzle")
	float AimTraceDistance = 20000.0f;

	// 총구의 월드 위치 (소켓 → 오프셋 → 액터 위치 순으로 폴백)
	FVector GetMuzzleLocation() const;
};