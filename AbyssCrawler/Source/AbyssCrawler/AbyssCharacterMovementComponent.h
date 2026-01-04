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

	// 외부(Character)에서 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Abyss Movement")
	void SetSprinting(bool bActive);

protected:
	// 현재 이동 모드에 따라 최대 속도를 반환하는 함수 오버라이드
	virtual float GetMaxSpeed() const override;
};