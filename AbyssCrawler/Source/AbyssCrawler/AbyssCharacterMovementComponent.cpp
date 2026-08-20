#include "AbyssCharacterMovementComponent.h"
#include "GameFramework/Character.h"

UAbyssCharacterMovementComponent::UAbyssCharacterMovementComponent()
{
	// 초기화
	bWantsToSprint = false;

	// CMC 기본 속도 설정 (지상 / 수중)
	MaxWalkSpeed = WalkSpeed;
	MaxSwimSpeed = SwimSpeed;

	// 기본 물리 설정
	BrakingDecelerationSwimming = 500.f; // 수중 감속 (낮을수록 오래 미끄러진다)
	Buoyancy = 1.0f; // 중성 부력
}

void UAbyssCharacterMovementComponent::SetSprinting(bool bActive)
{
	// 로컬(입력 머신)에서 플래그만 세운다.
	// 서버 전달은 SavedMove의 압축 플래그(FLAG_Custom_0)가 이동 패킷에 실어 자동 처리한다.
	bWantsToSprint = bActive;
}

// ──────────────────────────────────────────────────────────────
// 네트워크 예측: 스프린트 플래그 동기화
// ──────────────────────────────────────────────────────────────

// [서버] 클라 이동 패킷의 압축 플래그에서 스프린트 상태 복원
void UAbyssCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

// [클라] 예측 데이터를 커스텀 SavedMove 할당 버전으로 교체
FNetworkPredictionData_Client* UAbyssCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (!ClientPredictionData)
	{
		UAbyssCharacterMovementComponent* MutableThis = const_cast<UAbyssCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Abyss(*this);
	}
	return ClientPredictionData;
}

void FSavedMove_Abyss::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = 0;
}

uint8 FSavedMove_Abyss::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	return Result;
}

bool FSavedMove_Abyss::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	// 스프린트 상태가 다른 무브끼리 합치면 전환 시점이 뭉개지므로 금지
	if (bSavedWantsToSprint != static_cast<FSavedMove_Abyss*>(NewMove.Get())->bSavedWantsToSprint)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

// 무브 기록 시점: CMC 상태 → SavedMove
void FSavedMove_Abyss::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UAbyssCharacterMovementComponent* CMC = Cast<UAbyssCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsToSprint = CMC->bWantsToSprint;
	}
}

// 보정 리플레이 시점: SavedMove → CMC 상태 복원
void FSavedMove_Abyss::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	if (UAbyssCharacterMovementComponent* CMC = Cast<UAbyssCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		CMC->bWantsToSprint = bSavedWantsToSprint;
	}
}

float UAbyssCharacterMovementComponent::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const float HauntMul = FMath::Max(0.f, HauntSpeedMultiplier);

	switch (MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
		// 지상: 스프린트 중이면 RunSpeed, 아니면 WalkSpeed
		return (bWantsToSprint ? RunSpeed : WalkSpeed) * HauntMul;

	case MOVE_Swimming:
		// 수중: 스프린트 중이면 SwimDashSpeed, 아니면 SwimSpeed
		return (bWantsToSprint ? SwimDashSpeed : SwimSpeed) * HauntMul;

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
    // 1. 부모의 마찰과 브레이킹 감속을 끈다 (감속은 항력 하나로만 처리하기 위해)
    // =========================================================
    float NoBraking = 0.0f;
    float NoFriction = 0.0f;

    // =========================================================
    // 2. 목표 최고 속도에 정확히 수렴시키기 위한 '항력 계수'를 역산한다
    // =========================================================
    float TargetMaxSpeed = GetMaxSpeed(); // 수영 120, 대시 중이면 180 (cm/s)
    float CurrentMaxAccel = GetMaxAcceleration(); // 이 컴포넌트의 추진 가속도

    // [방어 코드] 0으로 나누는 것을 막는다
    float DragFactor = 0.0f;
    if (TargetMaxSpeed > KINDA_SMALL_NUMBER)
    {
        // 유도: k = a / v^2
        // 이렇게 두면 풀악셀을 유지해도 정확히 TargetMaxSpeed에서 가속도가 0이 된다
        DragFactor = CurrentMaxAccel / (TargetMaxSpeed * TargetMaxSpeed);
    }

    // =========================================================
    // 3. 속도의 제곱에 비례하는 항력(Drag)을 적용한다
    // =========================================================
    FVector CurrentVelocity = Velocity;
    float SpeedSq = CurrentVelocity.SizeSquared();

    if (SpeedSq > KINDA_SMALL_NUMBER)
    {
        // 위에서 구한 DragFactor로 항력의 크기를 만든다
        // 속도가 TargetMaxSpeed에 도달하면 이 항력이 추진력(Accel)과 정확히 상쇄된다
        FVector DragForce = -CurrentVelocity.GetSafeNormal() * (SpeedSq * DragFactor);

        // 속도에 반영
        Velocity += DragForce * DeltaTime;
    }

    // =========================================================
    // 4. 추진력(Acceleration)을 적용한다
    // =========================================================
    // 별도의 배율을 두지 않고 컴포넌트의 MaxAcceleration을 그대로 신뢰한다.
    // 여기서 추진력을 더 키우고 싶다면,
    // 위 DragFactor 계산에도 같은 값을 넣어야 수렴 지점이 어긋나지 않는다.

    if (Acceleration.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        Velocity += Acceleration * DeltaTime;
    }

    // =========================================================
    // 5. 최종 적용 (마찰과 브레이킹에 0을 넘겨 부모의 감속 모델을 끈다)
    // =========================================================
    Super::CalcVelocity(DeltaTime, NoFriction, bFluid, NoBraking);
}