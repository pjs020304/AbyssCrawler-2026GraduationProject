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

	// 기획서의 이동 속도 수치를 그대로 반영한다.
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

	// --- 수중 물리 파라미터 ---
	// 부가 질량 계수. 물속에서 몸을 밀 때 함께 밀리는 물의 양을 표현하려던 값이다.
	// (현재 CalcVelocity에서 사용하지 않는다. 항력 역산 방식으로 정리되면서 남은 값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float AddedMassCoefficient = 0.5f;

	// 물의 밀도. 위와 같은 이유로 현재 사용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float WaterDensity = 0.05f;	

	// 입력 가속도 배율(Thrust). 수중에서 앞으로 밀어 주는 힘의 배율이다.
	// (현재 사용하지 않는다. 추진력은 MaxAcceleration으로 직접 지정한다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float SwimmingAccelerationMultiplier = 2.5f;

	// 외부(Character)에서 스프린트를 켜고 끌 때 호출하는 함수
	UFUNCTION(BlueprintCallable, Category = "Abyss Movement")
	void SetSprinting(bool bActive);

	// [네트워크 예측] 클라 예측 데이터를 커스텀 SavedMove를 쓰는 버전으로 교체
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	// 현재 이동 모드에 맞는 최대 속도를 돌려주도록 오버라이드
	virtual float GetMaxSpeed() const override;

	// 매 프레임 가속도와 항력을 처리해 Velocity를 만드는 함수
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