#include "AbyssCharacterMovementComponent.h"

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
	bWantsToSprint = bActive;
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