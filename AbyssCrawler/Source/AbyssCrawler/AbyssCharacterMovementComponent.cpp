#include "AbyssCharacterMovementComponent.h"
#include "GameFramework/Character.h"

UAbyssCharacterMovementComponent::UAbyssCharacterMovementComponent()
{
	// �ʱ�ȭ
	bWantsToSprint = false;

	// CMC �⺻ ���� (������)
	MaxWalkSpeed = WalkSpeed;
	MaxSwimSpeed = SwimSpeed;

	// �⺻ ���� ����
	BrakingDecelerationSwimming = 500.f; // ���� ���� (�������� �̲�����)
	Buoyancy = 1.0f; // �߼� �η�
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
		// ����: Sprint ���̸� RunSpeed, �ƴϸ� WalkSpeed
		return (bWantsToSprint ? RunSpeed : WalkSpeed) * HauntMul;

	case MOVE_Swimming:
		// ����: Sprint ���̸� SwimDashSpeed, �ƴϸ� SwimSpeed
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
    // 1. ������ ���� ���� ���� (���� ���� ������ ����)
    // =========================================================
    float NoBraking = 0.0f;
    float NoFriction = 0.0f;

    // =========================================================
    // 2. ���� ������ �ִ� �ӵ��� �������� �ʿ��� '���� ���' ����
    // =========================================================
    float TargetMaxSpeed = GetMaxSpeed(); // �ȱ� ���� 120, ��� ���̸� 180 ��
    float CurrentMaxAccel = GetMaxAcceleration(); // ������Ʈ ������ (�⺻ 2048 ��)

    // [��� �ڵ�] 0���� ������ ����
    float DragFactor = 0.0f;
    if (TargetMaxSpeed > KINDA_SMALL_NUMBER)
    {
        // ����: k = a / v^2
        // �� ���� ����� �����ϸ�, Ǯ�Ǽ��� ��Ƶ� ��Ȯ�� TargetMaxSpeed���� ������ 0�� ��
        DragFactor = CurrentMaxAccel / (TargetMaxSpeed * TargetMaxSpeed);
    }

    // =========================================================
    // 3. ������ �׷�(Drag) ����
    // =========================================================
    FVector CurrentVelocity = Velocity;
    float SpeedSq = CurrentVelocity.SizeSquared();

    if (SpeedSq > KINDA_SMALL_NUMBER)
    {
        // ���� DragFactor�� ����Ͽ� ���׷� ����
        // �ӵ��� TargetMaxSpeed�� �����ϸ�, �� ���׷��� ��Ȯ�� ������(Accel)�� ������
        FVector DragForce = -CurrentVelocity.GetSafeNormal() * (SpeedSq * DragFactor);

        // ���� ����
        Velocity += DragForce * DeltaTime;
    }

    // =========================================================
    // 4. ������(Acceleration) ����
    // =========================================================
    // ������ Multiplier ���� ���� �⺻ MaxAcceleration�� �ŷ��ϰų�,
    // �ʿ��ϴٸ� ���⼭�� ��¦ ������ �� �ֽ��ϴ�. 
    // (��, �����Ѵٸ� �� DragFactor ������ ���ڿ��� ������ ���� ��� ��Ȯ�մϴ�)

    if (Acceleration.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        Velocity += Acceleration * DeltaTime;
    }

    // =========================================================
    // 5. ���� ����
    // =========================================================
    Super::CalcVelocity(DeltaTime, NoFriction, bFluid, NoBraking);
}