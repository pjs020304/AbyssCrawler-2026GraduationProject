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

protected:
	// 에디터에서 할당할 발사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AHarpoonProjectile> ProjectileClass;
};