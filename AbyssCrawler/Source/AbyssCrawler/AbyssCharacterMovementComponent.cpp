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

void UAbyssCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
    if (!IsSwimming())
    {
        MaxAcceleration = 1024;
        Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
        return;
    }
    MaxAcceleration = 512;
    // =========================================================
    // 1. 엔진의 강제 제동 제거 (순수 관성 주행을 위해)
    // =========================================================
    float NoBraking = 0.0f;
    float NoFriction = 0.0f;

    // =========================================================
    // 2. 현재 설정된 최대 속도를 기준으로 필요한 '저항 계수' 역산
    // =========================================================
    float TargetMaxSpeed = GetMaxSpeed(); // 걷기 모드면 120, 대시 중이면 180 등
    float CurrentMaxAccel = GetMaxAcceleration(); // 컴포넌트 설정값 (기본 2048 등)

    // [방어 코드] 0으로 나누기 방지
    float DragFactor = 0.0f;
    if (TargetMaxSpeed > KINDA_SMALL_NUMBER)
    {
        // 공식: k = a / v^2
        // 이 저항 계수를 적용하면, 풀악셀을 밟아도 정확히 TargetMaxSpeed에서 가속이 0이 됨
        DragFactor = CurrentMaxAccel / (TargetMaxSpeed * TargetMaxSpeed);
    }

    // =========================================================
    // 3. 물리적 항력(Drag) 적용
    // =========================================================
    FVector CurrentVelocity = Velocity;
    float SpeedSq = CurrentVelocity.SizeSquared();

    if (SpeedSq > KINDA_SMALL_NUMBER)
    {
        // 계산된 DragFactor를 사용하여 저항력 생성
        // 속도가 TargetMaxSpeed에 도달하면, 이 저항력이 정확히 추진력(Accel)과 같아짐
        FVector DragForce = -CurrentVelocity.GetSafeNormal() * (SpeedSq * DragFactor);

        // 저항 적용
        Velocity += DragForce * DeltaTime;
    }

    // =========================================================
    // 4. 추진력(Acceleration) 적용
    // =========================================================
    // 별도의 Multiplier 없이 엔진 기본 MaxAcceleration을 신뢰하거나,
    // 필요하다면 여기서만 살짝 증폭할 수 있습니다. 
    // (단, 증폭한다면 위 DragFactor 계산식의 분자에도 증폭된 값을 써야 정확합니다)

    if (Acceleration.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        Velocity += Acceleration * DeltaTime;
    }

    // =========================================================
    // 5. 엔진 위임
    // =========================================================
    Super::CalcVelocity(DeltaTime, NoFriction, bFluid, NoBraking);
}