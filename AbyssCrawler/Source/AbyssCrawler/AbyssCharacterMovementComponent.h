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

	// [네트워크 예측] 클라 예측 데이터를 커스텀 SavedMove를 쓰는 버전으로 교체
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	// ���� �̵� ��忡 ���� �ִ� �ӵ��� ��ȯ�ϴ� �Լ� �������̵�
	virtual float GetMaxSpeed() const override;

	// �� ������ ���ӵ�, ����, �극��ŷ�� ó���Ͽ� Velocity�� ���� �Լ�
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	// [네트워크 예측] 클라가 보낸 이동 패킷의 압축 플래그에서 스프린트 상태 복원 (서버 측)
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
};

/**
 * [네트워크 예측] 스프린트 플래그를 이동 패킷에 싣기 위한 SavedMove 확장.
 * 클라 입력(bWantsToSprint)이 FLAG_Custom_0으로 압축되어 서버로 전달되므로
 * 서버와 클라의 GetMaxSpeed()가 항상 일치한다 (고무줄 보정 방지).
 */
class FSavedMove_Abyss : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	// 이 무브가 기록될 때의 스프린트 입력 상태
	uint8 bSavedWantsToSprint : 1;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;
};

class FNetworkPredictionData_Client_Abyss : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	FNetworkPredictionData_Client_Abyss(const UCharacterMovementComponent& ClientMovement)
		: Super(ClientMovement) {}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(new FSavedMove_Abyss());
	}
};