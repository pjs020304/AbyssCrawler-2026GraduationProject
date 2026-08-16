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
class AAbyssUSBItem;
class AAbyssDataConsole;
class USoundBase;
class UMaterialInterface;

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

  UPROPERTY(EditAnywhere, Category = "Camera")
  FVector CameraEyeOffset = FVector::ZeroVector;

  // 피격 시 로컬 화면에 재생할 카메라 쉐이크 (BP에서 CS_HitReaction 지정)
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
  TSubclassOf<class UCameraShakeBase> HitCameraShake;

  // 피해량 → 쉐이크 강도 매핑 (X: 피해량 범위, Y: 강도 범위)
  UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
  FVector2D HitShakeDamageRange = FVector2D(1.f, 30.f);

  UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
  FVector2D HitShakeScaleRange = FVector2D(0.5f, 1.5f);

  // --- [문어에게 붙잡힌 동안] ---
  // 붙잡혀 있는 내내 지속 재생되는 쉐이크. Oscillation의 Duration을 0(=무한 루프)으로
  // 만든 쉐이크를 지정해야 한다. 탈출/사망 시 코드가 직접 정지시킨다.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
  TSubclassOf<class UCameraShakeBase> GrabbedCameraShake;

  UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
  float GrabbedShakeScale = 1.0f;

  // 탈출 연타(클릭)마다 한 번씩 터지는 짧은 쉐이크. "발버둥치는" 느낌용(선택).
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
  TSubclassOf<class UCameraShakeBase> GrabStruggleCameraShake;

  UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
  float GrabStruggleShakeScale = 1.0f;

  // --- [피라냐 군집에게 공격받는 동안] ---
  // 물릴 때마다(AttackInterval 주기) 한 번씩 터지는 짧고 날카로운 쉐이크.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
  TSubclassOf<class UCameraShakeBase> SwarmBiteCameraShake;

  UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
  float SwarmBiteShakeScale = 1.0f;

  // 군집에 둘러싸여 있는 동안 깔리는 지속 쉐이크(무한 루프 쉐이크 지정).
  // 물기 이벤트가 들어올 때 켜지고, SwarmShakeTimeout 동안 추가 공격이 없으면 꺼진다.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
  TSubclassOf<class UCameraShakeBase> SwarmAmbientCameraShake;

  UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
  float SwarmAmbientShakeScale = 1.0f;

  // 마지막 물기 이후 이 시간(초) 동안 조용하면 지속 쉐이크를 끈다.
  // 군집의 AttackInterval(기본 0.5초)보다 넉넉히 크게 잡을 것.
  UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
  float SwarmShakeTimeout = 1.5f;

  // --- 카메라 쉐이크 헬퍼 ---
  // 아래 3개는 모두 "내 화면의 주인(IsLocallyControlled)"일 때만 동작한다.
  // 이 체크가 없으면 리슨서버 호스트가 남의 피격에도 흔들린다.

  // 1회성 쉐이크 재생
  void PlayLocalCameraShake(TSubclassOf<class UCameraShakeBase> ShakeClass,
                            float Scale = 1.0f);

  // 지속형 쉐이크 시작. 이미 같은 핸들로 재생 중이면 중복 재생하지 않는다.
  void StartLoopingCameraShake(TSubclassOf<class UCameraShakeBase> ShakeClass,
                               float Scale,
                               TObjectPtr<class UCameraShakeBase> &InOutHandle);

  // 지속형 쉐이크 정지 (bImmediate=false면 자연스럽게 감쇠하며 종료)
  void StopLoopingCameraShake(TObjectPtr<class UCameraShakeBase> &InOutHandle,
                              bool bImmediate = false);

protected:
  // 현재 재생 중인 지속형 쉐이크 인스턴스. 정지시키려면 이 핸들이 필요하다.
  UPROPERTY(Transient)
  TObjectPtr<class UCameraShakeBase> ActiveGrabShake;

  UPROPERTY(Transient)
  TObjectPtr<class UCameraShakeBase> ActiveSwarmShake;

  // 마지막 물기 이후 SwarmShakeTimeout이 지나면 ActiveSwarmShake를 끈다.
  FTimerHandle SwarmShakeTimerHandle;

  void StopSwarmAmbientShake();

public:

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

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
  UInputAction* MissionUIAction; // 미션창

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

  // 아이템을 부착할 손 소켓 이름 (스켈레톤에 추가한 소켓)
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
  FName HandSocketName = TEXT("LeftHandSocket");

  // --- [탑승 포즈 (수중 스쿠터)] ---
  // UsesRidePose()가 true인 아이템을 장착하면 캐릭터 메시를 통째로 "플레이어 + 탈것"
  // 메시로 교체하고 전용 애니메이션을 재생한다. 두 메시가 같은 스켈레톤을 공유하므로
  // 손과 핸들의 정렬이 자동으로 맞고, 소켓/카메라도 그대로 유지된다.

  // 탑승 중에 사용할 스켈레탈 메시 (기본: RidingJet — 플레이어 + 제트기 일체형)
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ride Pose")
  TObjectPtr<class USkeletalMesh> RidePoseMesh;

  // 탑승 중 루프 재생할 애니메이션 (기본: RidingJet_Anim)
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ride Pose")
  TObjectPtr<class UAnimSequence> RidePoseAnim;

  // 메시 교체 방식을 쓸지 여부. false로 두면 메시는 그대로 두고 bIsRidingItem 플래그만
  // 갱신하므로, AB_DiverCharacter에서 직접 블렌딩하는 방식으로 전환할 수 있다.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ride Pose")
  bool bUseRideMeshSwap = true;

  // 물 밖(잠수함 내부 등)에서는 탑승 포즈를 쓰지 않는다
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ride Pose")
  bool bRidePoseRequiresSwimming = true;

  // 현재 탑승 포즈 상태. 복제된 Inventory/CurrentSlotIndex에서 파생되므로 모든 머신에서
  // 동일하게 계산된다 (별도 리플리케이션 불필요). AnimBP에서 읽어 쓸 수 있다.
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ride Pose")
  bool bIsRidingItem = false;

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

  void ToggleMissionPanel();

  // 미션 아이템 판정
  bool IsHoldingUSBItem() const;

  // 콘솔 미션 상호작용
  void StopInteract();

  // Console Data Recovery Mission
  UPROPERTY(EditDefaultsOnly, Category = "Mission|Console")
  TSubclassOf<AAbyssUSBItem> USBItemClass;

  void GiveConsoleMissionUSB();

  bool HasUSBItemInInventory() const;

  UFUNCTION(Server, Reliable)
  void Server_StartDataConsole(AAbyssDataConsole* Console);

  UFUNCTION(Server, Reliable)
  void Server_StopDataConsole(AAbyssDataConsole* Console);

  UFUNCTION(Client, Reliable)
  void Client_SetWorkInputBlocked(bool bBlocked);

  UFUNCTION(Client, Reliable)
  void Client_SetMovementBlocked(bool bBlocked);

  void ConsumeItem(AAbyssItemBase *ItemToConsume);

  // 피라냐 군체에 물렸을 때 해당 플레이어의 클라이언트에서 호출 (카메라 흔들림/화면 효과용)
  UFUNCTION(Client, Unreliable)
  void Client_OnSwarmBite(FVector SwarmCenter);

  // 실제 연출은 블루프린트에서 구현 (카메라 셰이크, 포스트프로세스 등)
  UFUNCTION(BlueprintImplementableEvent, Category = "Feedback")
  void OnSwarmBiteFeedback(FVector SwarmCenter);

  // --- [유령(원혼) 디버프 / 포획] ---

  // 유령 근접 디버프 단계 (0=없음, 1=둔화, 2=시야왜곡, 3=암전). 서버 권위, 복제됨.
  UPROPERTY(ReplicatedUsing = OnRep_GhostHauntStage, VisibleAnywhere,
            BlueprintReadOnly, Category = "Ghost")
  int32 GhostHauntStage = 0;

  // 서버 전용 근접 누적 시간 (디렉터가 갱신). 복제 안 함.
  float GhostHauntTime = 0.f;

  // 포획되어 즉사 진행 중인지
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
  bool bCaughtByGhost = false;

  // 단계별 이동 둔화 배율 (인덱스 = 단계). 에디터에서 조정 가능.
  UPROPERTY(EditDefaultsOnly, Category = "Ghost")
  TArray<float> GhostStageSpeedMultipliers = {1.0f, 0.6f, 0.5f, 0.45f};

  // 포획 후 암전 → 사망까지 지연(초)
  UPROPERTY(EditDefaultsOnly, Category = "Ghost")
  float GhostKillDelay = 1.5f;

  FTimerHandle GhostKillTimerHandle;

  UFUNCTION()
  void OnRep_GhostHauntStage();

  // 서버에서 디렉터가 호출: 디버프 단계 설정
  void SetGhostHauntStage(int32 NewStage);

  // 서버에서 디렉터가 호출: 유령에게 포획됨 → 암전 후 사망
  void CaughtByGhost();

  UFUNCTION(NetMulticast, Reliable)
  void Multicast_CaughtByGhost();

  // 현재 디버프 단계에 맞춰 이동 둔화 적용 (서버/클라 공통)
  void ApplyGhostHauntMovement();

  // 화면 연출 훅 (BP에서 구현: 1=둔화 표시, 2=시야 왜곡, 3=암전)
  UFUNCTION(BlueprintImplementableEvent, Category = "Ghost")
  void OnGhostHauntStageChanged(int32 NewStage);

  // 포획 즉사 시 완전 암전 연출 (BP에서 구현)
  UFUNCTION(BlueprintImplementableEvent, Category = "Ghost")
  void OnGhostKillBlackout();

  // 잠수함 탑승/하차 시 외부(잠수함)에서 호출해 줄 함수
  void SetInsideSubmarine(bool bInside);

  void SetCurrentWorkObject(AAbyssMissionWorkObject *WorkObject);
  void ClearCurrentWorkObject(AAbyssMissionWorkObject *WorkObject);
  void CancelCurrentWorkByEnemyAttack();

  UPROPERTY()
  bool bIsWorkingLocked = false;

  // 사망 상태. 리플리케이트되어 클라 관전 필터/후발 조인자 사망 표시가 정상 작동한다.
  UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category = "State")
  bool bIsDead = false;

  UFUNCTION()
  void OnRep_IsDead();

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
  bool bIsHoldingItem = false;

  UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "State")
  EAbyssWorkType CurrentWorkType = EAbyssWorkType::None;

  UFUNCTION(BlueprintCallable, Category = "State")
  void SetWorkType(EAbyssWorkType NewWorkType);

  void Die();

  UFUNCTION(Server, Reliable)
  void Server_Die();

  // 사망 "순간" 연출 전용 (사운드/파티클 등). 상태 적용은 OnRep_IsDead/ApplyDeadVisuals가 담당.
  UFUNCTION(NetMulticast, Reliable)
  void Multicast_Die();

  // 사망 상태의 비주얼/콜리전 적용 (멱등). 서버는 직접, 클라는 OnRep_IsDead에서 호출.
  void ApplyDeadVisuals();

  void SetInputLockedByUI(bool bLocked);

	bool HasEnoughEmptyInventorySlots(int32 NeededSlots) const;
	void ApplyCorpseCarryPenalty();
	void RemoveCorpseCarryPenalty();

	// [서버] 들고 있는 모든 아이템을 현재 위치에 드롭.
	// 부활 장치가 죽은 캐릭터 껍데기를 제거하기 전에 호출 (아이템 소실 방지)
	void DropAllInventoryItems();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> FadeWidgetClass;

    UPROPERTY()
    UUserWidget* FadeWidget = nullptr;

    // drop sound
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
    TObjectPtr<USoundBase> ItemDropSound;
    
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayItemDropSound(FVector SoundLocation);

    // Chatting
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* ChatAction;

    void OpenChatInput();

    void SendChatMessage(const FString& Message);

    UFUNCTION(Server, Reliable)
    void Server_SendChatMessage(const FString& Message);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveChatMessage(const FString& Message);

    // Clear & Over UI
    UFUNCTION(Client, Reliable)
    void Client_ShowGameClearUI();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> GameOverWidgetClass;

    UPROPERTY()
    TObjectPtr<UUserWidget> GameOverWidgetRef;

    // Fade out
    UFUNCTION(Client, Reliable)
    void Client_PlayFadeOut();

private:
  UPROPERTY()
  AAbyssMissionWorkObject *CurrentWorkObject = nullptr;

  bool bInputLockedByUI = false;

  UPROPERTY()
  TObjectPtr<AAbyssDataConsole> CurrentDataConsole = nullptr;

  // Debug
  void Debug_SetProgressFull();
  void Debug_SetRemainingTime10();

  public:
      void ApplyPlayerSuitMaterial();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Suit")
    TArray<TObjectPtr<UMaterialInterface>> SuitMaterialVariants;

    void TrySubmitColorFromGameInstance();

    UFUNCTION(Server, Reliable)
    void Server_SubmitPlayerColorIndex(int32 ColorIndex);

    FTimerHandle SubmitColorTimerHandle;

public:
    virtual void OnRep_PlayerState() override;

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

  // 매 프레임 장착 아이템/이동 상태를 보고 탑승 포즈 진입·이탈을 판정 (상태가 바뀔 때만 적용)
  void UpdateRidePose();

  // 실제 메시/애니메이션 전환 수행
  void SetRidePoseActive(bool bActive);

  // 탑승 포즈 진입 전의 원본 메시/AnimBP (복귀용). BeginPlay에서 캐싱.
  UPROPERTY(Transient)
  TObjectPtr<class USkeletalMesh> DefaultBodyMesh;

  UPROPERTY(Transient)
  TSubclassOf<class UAnimInstance> DefaultAnimClass;

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

  UFUNCTION(Client, Reliable)
  void Client_ShowGameOverUI();

  UFUNCTION(Client, Unreliable)
  void Client_PlaySound2D(USoundBase* Sound);
};