#pragma once

#include "AbilitySystemInterface.h"
#include "AbyssInteractionInterface.h"
#include "AbyssItemBase.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "InputActionValue.h"
#include "AbyssDiverCharacter.generated.h"



// 블루프린트(UI)로 신호를 Delegate 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceChanged, float,
                                             CurrentValue, float, MaxValue);

// 전방 선언
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;
class UAbyssCharacterMovementComponent;
class UAbilitySystemComponent;
class UAbyssAttributeSet;
class UParticleSystemComponent;
class APostProcessVolume;
class AAbyssItemBase;
class UMissionSelectUIWidget;
class AAbyssMissionSender;
struct FAbyssMissionData;
class AAbyssMissionWorkObject;
class UMainHUDWidget;

UENUM(BlueprintType)
enum class EAbyssWorkType : uint8
{
	None UMETA(DisplayName = "None"),
	MissionWork UMETA(DisplayName = "Mission Work"),
	ItemInstall UMETA(DisplayName = "Item Install")
};

UCLASS()
class ABYSSCRAWLER_API AAbyssDiverCharacter : public ACharacter, public IAbilitySystemInterface {
  GENERATED_BODY()

public:
  // 커스텀 무브먼트 컴포넌트 사용을 위한 생성자 선언
  AAbyssDiverCharacter(const FObjectInitializer &ObjectInitializer);

  virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

protected:
  virtual void BeginPlay() override;

public:
  virtual void Tick(float DeltaTime) override;
  virtual void SetupPlayerInputComponent(
      class UInputComponent *PlayerInputComponent) override;
  // 리플리케이션 설정 (변수 동기화)
  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty> &OutLifetimeProps) const override;

  // --- Components ---
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
  UCameraComponent *FirstPersonCameraComponent;

  // 심해 수중 부유물(Marine Snow)을 위한 GPU 연산 파티클 컴포넌트
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
  UParticleSystemComponent* MarineSnowParticleComponent;

  // 심해 포스트 프로세스 볼륨 캐싱
  UPROPERTY(Transient)
  APostProcessVolume* DeepSeaPPVolume;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment|PostProcess")
  float PPBlendSpeed = 2.0f; // 보간 속도

  float CurrentPPWeight = 0.0f;
  float TargetPPWeight = 0.0f;

  // 몸체 메쉬 (그림자 및 멀티플레이 타인 시점용)
  // (참고: ACharacter의 기본 GetMesh()를 사용하되, 1인칭용 팔 메쉬를 따로 둘
  // 수도 있음)

  // --- Inputs ---
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputMappingContext *DefaultMappingContext;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *MoveAction; // WASD

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *LookAction; // Mouse XY

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *JumpAction; // Space (수중 상승)

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *CrouchAction; // Ctrl (수중 하강)

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *DashAction; // Shift 키 매핑

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *ConvertAction; // C키로 전환

  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  bool IsSwimming = true;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *UseItemAction; // 마우스 좌클릭

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *Slot1Action; // 숫자키 1

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *Slot2Action; // 숫자키 2

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *Slot3Action; // 숫자키 3

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *Slot4Action; // 숫자키 4

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *Slot5Action; // 숫자키 5

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *InteractAction; // 상호작용키 E

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction *DropAction; // 버리기키 Q

  UPROPERTY(EditAnywhere, Category = "Interaction")
  float InteractDistance = 250.0f;

  // --- Inventory System ---
  // 아이템 슬롯 (최대 5개)
  UPROPERTY(ReplicatedUsing = OnRep_Inventory, VisibleInstanceOnly,
            BlueprintReadOnly, Category = "Inventory")
  TArray<AAbyssItemBase *> Inventory;

  // 현재 선택된 슬롯 번호 (0 ~ 4)
  // UI(블루프린트)에서 이 값을 읽어갈 수 있도록 허락
  UPROPERTY(ReplicatedUsing = OnRep_CurrentSlotIndex, VisibleAnywhere,
            BlueprintReadOnly, Category = "Inventory")
  int32 CurrentSlotIndex;

  // 시작 시 지급할 아이템 클래스 (에디터에서 설정)
  UPROPERTY(EditAnywhere, Category = "Inventory")
  TSubclassOf<AAbyssItemBase> DefaultItemClass;

  // 현재 시선이 머물고 있는 상호작용 액터 기억하기
  UPROPERTY()
  AActor *FocusedActor;

  UFUNCTION()
  void OnRep_Inventory();

  UFUNCTION()
  void OnRep_CurrentSlotIndex();

  // --- public 함수 ---
  UFUNCTION(BlueprintImplementableEvent, Category = "UI")
  void OnInventoryUpdated();

  bool AddItemToInventory(AAbyssItemBase *AddedItem);

  // 매 프레임 시선을 검사하는 함수
  void CheckForInteractables();

  // 산소가 변할 때 블루프린트로 쏴줄 이벤트
  UPROPERTY(BlueprintAssignable, Category = "Abilities|UI")
  FOnResourceChanged OnOxygenChanged;

  // 에디터에 GE_DrainOxygen을 넣을 칸
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
  TSubclassOf<class UGameplayEffect> OxygenDrainEffectClass;

  // 체력이 변할 때 블루프린트로 쏴줄 이벤트
  UPROPERTY(BlueprintAssignable, Category = "Abilities|UI")
  FOnResourceChanged OnHealthChanged;

  // 배터리가 변할 때 블루프린트로 쏴줄 이벤트
  UPROPERTY(BlueprintAssignable, Category = "Abilities|UI")
  FOnResourceChanged OnBatteryChanged;

  void RefreshEquippedVisual();

  // 미션 아이템 수집 클라 -> 서버 요청
  UFUNCTION(Server, Reliable)
  void Server_OnItemCollected();

  UPROPERTY(BlueprintReadWrite, Category = "UI")
  UMainHUDWidget *MainHUDRef;

  UFUNCTION(Client, Reliable)
  void Client_ShowWorkUI();

  UFUNCTION(Client, Reliable)
  void Client_HideWorkUI();

  UFUNCTION(Client, Reliable)
  void Client_UpdateWorkProgress(float Progress);

  UFUNCTION(Client, Reliable)
  void Client_ShowMissionComplete(const FText &MissionName);

  bool HasEmptyInventorySlot() const;

  UFUNCTION(Server, Reliable)
  void Server_AcceptMissionById(FName MissionId);

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
  TSubclassOf<UMissionSelectUIWidget> MissionSelectUIClass;

  UFUNCTION(Client, Reliable)
  void
  Client_OpenMissionSelectUI(const TArray<FAbyssMissionData> &AvailableMissions,
                             AAbyssMissionSender *MissionSender);

  UFUNCTION(Server, Reliable)
  void Server_ReleaseMissionSender(AAbyssMissionSender *MissionSender);

  UPROPERTY()
  UMissionSelectUIWidget *MissionSelectUIRef = nullptr;

  void ClearMissionSelectUIRef();

  UFUNCTION(Client, Reliable)
  void Client_SetWorkInputBlocked(bool bBlocked);

  UFUNCTION(Client, Reliable)
  void Client_SetMovementBlocked(bool bBlocked);

  void ConsumeItem(AAbyssItemBase *ItemToConsume);

  // 잠수함 탑승/하차 시 외부(잠수함)에서 호출해 줄 함수
  void SetInsideSubmarine(bool bInside);

  void SetCurrentWorkObject(AAbyssMissionWorkObject *WorkObject);
  void ClearCurrentWorkObject(AAbyssMissionWorkObject *WorkObject);
  void CancelCurrentWorkByEnemyAttack();

  UPROPERTY()
  bool bIsWorkingLocked = false;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
  bool bIsDead = false;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
  bool bIsHoldingItem = false;

  UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "State")
  EAbyssWorkType CurrentWorkType = EAbyssWorkType::None;

  UFUNCTION(BlueprintCallable, Category = "State")
  void SetWorkType(EAbyssWorkType NewWorkType);

  void Die();

  UFUNCTION(Server, Reliable)
  void Server_Die();

  UFUNCTION(NetMulticast, Reliable)
  void Multicast_Die();

  void SetInputLockedByUI(bool bLocked);

	bool HasEnoughEmptyInventorySlots(int32 NeededSlots) const;
	void ApplyCorpseCarryPenalty();
	void RemoveCorpseCarryPenalty();

private:
  UPROPERTY()
  AAbyssMissionWorkObject *CurrentWorkObject = nullptr;

  bool bInputLockedByUI = false;

protected:
  // 이동 함수
  void Move(const FInputActionValue &Value);
  void Look(const FInputActionValue &Value);
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
  void StopUseCurrentItem();

  // 슬롯 변경 (1, 2번 키)
  void EquipSlot1();
  void EquipSlot2();
  void EquipSlot3();
  void EquipSlot4();
  void EquipSlot5();

  // 내부적으로 슬롯 바꾸는 함수
  void SwitchToSlot(int32 NewIndex);

  // 서버 슬롯 전환
  UFUNCTION(Server, Reliable)
  void Server_SwitchToSlot(int32 NewIndex);

  void ApplyCurrentSlotVisual();

  // 상호작용 시도 함수
  void TryInteract();

  // 클라 -> 서버 호출
  // Reliable : 패킷 전송 보장, 서버에서만 실행
  // 클라에서 어떤 동작을 서버에서 요청 할 때 사용
  UFUNCTION(Server, Reliable)
  void Server_TryInteract(AActor *TargetActor);

  void DropItem();

  UFUNCTION(Server, Reliable)
  void Server_DropItem();

  UFUNCTION(Server, Reliable)
  void Server_UseCurrentItem();

  UFUNCTION(Server, Reliable)
  void Server_StopUseCurrentItem();

  UFUNCTION(Client, Reliable)
  void Client_OnInventoryUpdated();

  // --- [GAS Components] ---
  // GAS의 심장 (모든 스킬과 이펙트를 처리)
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities",
            meta = (AllowPrivateAccess = "true"))
  UAbilitySystemComponent *AbilitySystemComponent;

  // 자원 보관소 (체력, 산소, 배터리)
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities",
            meta = (AllowPrivateAccess = "true"))
  UAbyssAttributeSet *AttributeSet;

  // --- [GAS 초기화 함수들] ---
  // 서버에서 캐릭터를 조종하기 시작할 때 호출
  virtual void PossessedBy(AController *NewController) override;
  // GAS 내부에서 산소 값이 변하면 자동으로 실행될 콜백 함수

  void OnOxygenChangedCallback(const struct FOnAttributeChangeData &Data);

  void OnHealthChangedCallback(const struct FOnAttributeChangeData &Data);

  void OnBatteryChangedCallback(const struct FOnAttributeChangeData &Data);

  void SetupEnhancedInput();
  virtual void OnRep_Controller() override;

public:
  // --- Grab Mechanics ---
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grab")
  bool bIsGrabbed = false;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grab")
  int32 CurrentEscapeClicks = 0;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grab")
  int32 RequiredEscapeClicks = 10;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grab")
  ACharacter *CurrentGrabber = nullptr;

  FTimerHandle GrabDOTTimerHandle;
  float GrabDamagePerSecond = 3.0f;

  void OnGrabbed(ACharacter *Grabber, int32 RequiredClicks,
                 float DamagePerSecond);

  UFUNCTION(Server, Reliable)
  void Server_OnGrabbed(ACharacter *Grabber, int32 RequiredClicks,
                        float DamagePerSecond);

  UFUNCTION(NetMulticast, Reliable)
  void Multicast_OnGrabbed(ACharacter *Grabber);

  void EscapeGrab();

  UFUNCTION(Server, Reliable)
  void Server_EscapeGrab();

  UFUNCTION(NetMulticast, Reliable)
  void Multicast_EscapeGrab();

  UFUNCTION()
  void ApplyGrabDamage();
};