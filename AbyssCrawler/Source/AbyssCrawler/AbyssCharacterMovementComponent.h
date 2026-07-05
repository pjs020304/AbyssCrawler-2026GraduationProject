#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbyssCharacterMovementComponent.generated.h"

UCLASS()
class ABYSSCRAWLER_API UAbyssCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UAbyssCharacterMovementComponent();

	// ��ȹ�� �̵� �ӵ� ���� �ݿ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Movement")
	float WalkSpeed = 150.f; // 1.5m/s -> 150cm/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Movement")
	float RunSpeed = 225.f;  // 2.25m/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Movement")
	float SwimSpeed = 120.f; // 1.2m/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Movement")
	float SwimDashSpeed = 180.f; // 1.8m/s

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Abyss Movement")
	bool bWantsToSprint;

	// 유령 디버프 등 외부 둔화 배율 (1 = 정상). GetMaxSpeed에 곱해진다.
	UPROPERTY(BlueprintReadWrite, Category = "Abyss Movement")
	float HauntSpeedMultiplier = 1.f;

	// --- ���� ���� ���� ---
	// �ΰ� ���� ��� (�������� ������ ����)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float AddedMassCoefficient = 0.5f;

	// ���� �е��� 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float WaterDensity = 0.05f;	

	// [Ʃ�� �ٽ� 2] �Է� ���ӵ� ���� (Thrust)
	// ���ӿ��� �������ϴ� ���Դϴ�. ������ �հ� ������ ���� ũ���Դϴ�.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float SwimmingAccelerationMultiplier = 2.5f;

	// �ܺ�(Character)���� ȣ���� �Լ�
	UFUNCTION(BlueprintCallable, Category = "Abyss Movement")
	void SetSprinting(bool bActive);

protected:
	// ���� �̵� ��忡 ���� �ִ� �ӵ��� ��ȯ�ϴ� �Լ� �������̵�
	virtual float GetMaxSpeed() const override;

	// �� ������ ���ӵ�, ����, �극��ŷ�� ó���Ͽ� Velocity�� ���� �Լ�
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
};