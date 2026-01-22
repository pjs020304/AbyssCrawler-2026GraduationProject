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

	// 기획서 이동 속도 설계 반영
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

	// --- 수중 물리 설정 ---
	// 부가 질량 계수 (높을수록 반응이 굼뜸)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float AddedMassCoefficient = 0.5f;

	// 물의 밀도값 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float WaterDensity = 0.05f;	

	// [튜닝 핵심 2] 입력 가속도 증폭 (Thrust)
	// 물속에서 발차기하는 힘입니다. 저항을 뚫고 나가는 힘의 크기입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abyss Physics")
	float SwimmingAccelerationMultiplier = 2.5f;

	// 외부(Character)에서 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Abyss Movement")
	void SetSprinting(bool bActive);

protected:
	// 현재 이동 모드에 따라 최대 속도를 반환하는 함수 오버라이드
	virtual float GetMaxSpeed() const override;

	// 매 프레임 가속도, 마찰, 브레이킹을 처리하여 Velocity를 결정 함수
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
};