#include "AbyssSharkAnimInstance.h"
#include "AbyssSharkCharacter.h"

void UAbyssSharkAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SharkOwner = Cast<AAbyssSharkCharacter>(TryGetPawnOwner());
}

void UAbyssSharkAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 에디터 프리뷰 등에서 Initialize 시점에 폰이 없을 수 있으므로 재시도
	if (!SharkOwner)
	{
		SharkOwner = Cast<AAbyssSharkCharacter>(TryGetPawnOwner());
		if (!SharkOwner)
		{
			return;
		}
	}

	bIsAttacking = SharkOwner->IsAttacking();
	bIsDead = SharkOwner->IsDead();
	Speed = SharkOwner->GetVelocity().Size();
}
