#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AbyssSharkAnimInstance.generated.h"

class AAbyssSharkCharacter;

/**
 * AB_Shark의 부모 클래스.
 * State Machine의 Transition Rule에서 아래 변수들을 바로 사용하면 된다.
 *  - bIsAttacking : Swim -> Attack 전이 조건
 *  - bIsDead      : 어떤 상태에서든 -> Death 전이 조건
 *  - Speed        : Idle/Swim 블렌드용 (BS_Shark)
 */
UCLASS()
class ABYSSCRAWLER_API UAbyssSharkAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 소유 캐릭터 캐시
	UPROPERTY(Transient)
	TObjectPtr<AAbyssSharkCharacter> SharkOwner;

	// 공격 어빌리티가 활성화된 동안 true
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shark", meta = (AllowPrivateAccess = "true"))
	bool bIsAttacking = false;

	// 체력이 0이 되어 사망하면 true (다시 false가 되지 않음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shark", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	// 현재 이동 속력 (cm/s)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shark", meta = (AllowPrivateAccess = "true"))
	float Speed = 0.0f;
};
