#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbyssDiverCharacter.generated.h"

// 전방 선언
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;
class UAbyssCharacterMovementComponent;

UCLASS()
class ABYSSCRAWLER_API AAbyssDiverCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 커스텀 무브먼트 컴포넌트 사용을 위한 생성자 선언
	AAbyssDiverCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* FirstPersonCameraComponent;

	// 몸체 메쉬 (그림자 및 멀티플레이 타인 시점용)
	// (참고: ACharacter의 기본 GetMesh()를 사용하되, 1인칭용 팔 메쉬를 따로 둘 수도 있음)

	// --- Inputs ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction; // WASD

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction; // Mouse XY

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction; // Space (수중 상승)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* CrouchAction; // Ctrl (수중 하강)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* DashAction; // Shift 키 매핑 예정

protected:
	// 이동 함수
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartAscend(); // 상승 (점프)
	void StopAscend();
	void StartDescend(); // 하강 (크런치)
	void StopDescend();
	void StartDash();
	void StopDash();
};