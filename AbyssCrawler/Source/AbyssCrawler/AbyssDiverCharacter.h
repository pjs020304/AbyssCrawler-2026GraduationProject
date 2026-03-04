#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbyssItemBase.h" 
#include "AbyssInteractionInterface.h"
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
	// 리플리케이션 설정 (변수 동기화)
	//virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	UInputAction* DashAction; // Shift 키 매핑

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ConvertAction;	// C키로 전환

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsSwimming = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* UseItemAction; // 마우스 좌클릭

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* Slot1Action; // 숫자키 1

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* Slot2Action; // 숫자키 2

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* Slot3Action; // 숫자키 3

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* Slot4Action; // 숫자키 4

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* Slot5Action; // 숫자키 5

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* InteractAction; // 상호작용키 E

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* DropAction; // 버리기키 Q

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractDistance = 250.0f;

	// --- Inventory System ---
	// 아이템 슬롯 (최대 5개)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<AAbyssItemBase*> Inventory;

	// 현재 선택된 슬롯 번호 (0 ~ 4)
	// UI(블루프린트)에서 이 값을 읽어갈 수 있도록 허락
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 CurrentSlotIndex;

	// 시작 시 지급할 아이템 클래스 (에디터에서 설정)
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<AAbyssItemBase> DefaultItemClass;

	// 현재 시선이 머물고 있는 상호작용 액터 기억하기
	UPROPERTY()
	AActor* FocusedActor;

	// --- public 함수 ---
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnInventoryUpdated();

	bool AddItemToInventory(AAbyssItemBase* AddedItem);

	// 매 프레임 시선을 검사하는 함수
	void CheckForInteractables();


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

	// 수중 이동 전환
	void ConvertMove();

	// 아이템 사용 (클릭)
	void UseCurrentItem();

	// 슬롯 변경 (1, 2번 키)
	void EquipSlot1();
	void EquipSlot2();
	void EquipSlot3();
	void EquipSlot4();
	void EquipSlot5();

	// 내부적으로 슬롯 바꾸는 함수
	void SwitchToSlot(int32 NewIndex);

	// 상호작용 시도 함수
	void TryInteract();

	void DropItem();
};