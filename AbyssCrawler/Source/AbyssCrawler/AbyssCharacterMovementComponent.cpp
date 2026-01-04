#include "AbyssCharacterMovementComponent.h"

UAbyssCharacterMovementComponent::UAbyssCharacterMovementComponent()
{
	// 초기화
	bWantsToSprint = false;

	// CMC 기본 세팅 (참고용)
	MaxWalkSpeed = WalkSpeed;
	MaxSwimSpeed = SwimSpeed;

	// 기본 수영 설정
	BrakingDecelerationSwimming = 500.f; // 관성 느낌 (낮을수록 미끄러짐)
	Buoyancy = 1.0f; // 중성 부력
}

void UAbyssCharacterMovementComponent::SetSprinting(bool bActive)
{
	bWantsToSprint = bActive;
}

float UAbyssCharacterMovementComponent::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	switch (MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
		// 지상: Sprint 중이면 RunSpeed, 아니면 WalkSpeed
		return bWantsToSprint ? RunSpeed : WalkSpeed;

	case MOVE_Swimming:
		// 수중: Sprint 중이면 SwimDashSpeed, 아니면 SwimSpeed
		return bWantsToSprint ? SwimDashSpeed : SwimSpeed;

	default:
		return MaxSpeed;
	}
}