// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h" // 태그 사용을 위해 필요
#include "AbyssItemBase.generated.h"

class AAbyssDiverCharacter;

UCLASS()
class ABYSSCRAWLER_API AAbyssItemBase : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	AAbyssItemBase();

	virtual void PostInitializeComponents() override;
	
	virtual void UseItem();
	virtual void EndUseItem();

	// 지금 이 아이템을 사용할 수 있는지 (쿨타임 등). 기본은 항상 true.
	//
	// 두 곳에서 불린다:
	//  - 클라이언트: 입력 시점에 연출/서버 요청을 걸러내는 예측용
	//  - 서버      : UseItem() 안에서의 최종 권위 판정
	// 그래서 구현체는 자기가 서버인지 클라인지에 맞는 타임스탬프를 봐야 한다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual bool CanUseItem() const { return true; }

	// 클라이언트가 "지금 썼다"고 로컬에 기록해 두는 훅 (쿨타임 예측용).
	// 서버에서는 UseItem()이 직접 기록하므로 보통 아무것도 하지 않는다.
	virtual void NotifyUseAttempted() {}

	// 슬롯에서 해제될 때 캐릭터에서 호출 (예: 슬롯 전환, 드롭)
	virtual void NotifyUnequipped();

	// 이 아이템을 장착했을 때 캐릭터를 "손에 드는" 포즈가 아니라 전용 탑승 포즈로 전환할지.
	// true면 캐릭터가 RidePoseMesh/RidePoseAnim으로 바뀐다.
	// 수중 스쿠터가 유일한 사용처이며, 다른 탈것 아이템이 생기면 여기서 override하면 된다.
	virtual bool UsesRidePose() const { return false; }

	// --- 탑승 포즈일 때의 아이템 메시 배치 ---
	// 탑승 포즈에서는 캐릭터가 다른 메시/애니메이션으로 바뀌므로 손 위치가 달라진다.
	// 아래 값들로 탑승 중에만 적용할 부착 위치를 따로 지정한다.

	// 탑승 메시 쪽에 아이템 형상이 이미 포함되어 있다면 true로 두어 중복 표시를 막는다.
	// (RidingJet 메시에는 제트기 형상이 없으므로 기본값은 false)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ride Pose")
	bool bHideMeshInRidePose = false;

	// 탑승 중 부착할 소켓. None이면 캐릭터의 HandSocketName을 그대로 쓴다.
	// 주의: 탑승 중에는 스켈레톤이 Walk_in_Water_Skeleton이므로 소켓도 그쪽에 추가해야 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ride Pose")
	FName RideAttachSocketName = NAME_None;

	// 소켓 기준 추가 오프셋. 에디터에서 눈으로 보며 위치/회전을 맞추는 용도.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ride Pose")
	FTransform RideAttachOffset;

	UPROPERTY(EditDefaultsonly, BlueprintReadOnly, Category = "Item Info")
	FString ItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* ItemIcon;

	// 루트 컴포넌트. 충돌/물리/상호작용 트레이스를 담당한다.
	// (메시를 루트로 두면 BP에서 메시 회전/이동이 불가능해서 분리함 — 각 아이템 BP에서 Extent를 메시 크기에 맞게 조절할 것)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* CollisionComp;

	// 시각용 메시. 루트가 아니므로 BP에서 자유롭게 회전/이동/스케일 조절 가능 (충돌 없음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category =  "Item Info")
	int32 ItemPrice;

	// 이 아이템 사용 시 로컬 화면에 재생할 카메라 쉐이크 (각 아이템 BP에서 지정, 비우면 없음)
	// 재생은 서버가 아닌 "입력 시점"(AAbyssDiverCharacter::UseCurrentItem)에서 로컬로 수행된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
	TSubclassOf<class UCameraShakeBase> UseCameraShake;

	virtual void Interact_Implementation(AActor* InstigatorActor) override;

	// 3D 허공에 띄울 UI 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* InteractWidgetComp;

	// 인터페이스 오버라이드
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

	// 현재 누군가의 인벤토리에 들어있는지 (판매존 등 외부에서 판매 가능 여부 판단용)
	bool IsPickedUp() const { return bPickedUp; }

	// 아이템 상태 전환
	void SetAsPickedUp(AAbyssDiverCharacter* NewOwnerCharacter, USceneComponent* AttachParent, bool bVisibleInHand, FName AttachSocketName = NAME_None);
	void SetAsDropped(const FVector& DropLocation, const FRotator& DropRotation, const FVector& ThrowImpulse);

protected:

	UPROPERTY()
	AAbyssDiverCharacter* OwnerCharacter = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_PickedUp, VisibleAnywhere, BlueprintReadOnly, Category = "Item State")
	bool bPickedUp = false;

	UFUNCTION()
	void OnRep_PickedUp();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ApplyPickedUpState();

	// 이 아이템을 사용할 때 소모될 배터리 양 (UseItem 호출 시 1회 차감)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Cost")
	float BatteryCost;

	// 초당 지속 소모량. 0이면 지속 소모 없음 (손전등: ~2, 수중추진기: ~5)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Cost")
	float DrainRatePerSecond = 0.0f;

	// 배터리를 깎을 공용 이펙트 클래스 (GE_ConsumeBattery)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Cost")
	TSubclassOf<class UGameplayEffect> BatteryConsumeEffectClass;

	// SetByCaller로 값을 넘겨주기 위한 연결 고리 태그 (예: "Data.Cost.Battery")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Cost")
	FGameplayTag BatteryCostTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> PickupSound;

	// 아이템을 사용했을 때 재생할 효과음.
	// 사용이 실제로 성사된 순간에만 울린다(배터리 부족 등으로 취소되면 재생하지 않음).
	// 손전등 / 수중 추진기처럼 켜고 끄는 아이템은 이 값 대신 각자의 전용 사운드를 쓴다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> UseSound;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayPickupSound();

	// 지정한 효과음을 모든 머신에서 아이템 위치에 재생한다.
	// 서버에서만 의미가 있으며(권위), 서버가 아니거나 Sound가 비어 있으면 아무것도 하지 않는다.
	// 아이템은 손에 부착되어 있으므로 재생 위치는 곧 사용자의 손 위치가 된다.
	void PlayItemSound(USoundBase* Sound);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayItemSound(USoundBase* Sound);

	// --- 지속 소모 (Passive Drain) ---

	// 지속 소모 타이머 핸들
	FTimerHandle PassiveDrainTimer;

	// 지속 소모 타이머 시작 (서버 전용, DrainRatePerSecond > 0일 때만 유효)
	void StartPassiveDrain();

	// 지속 소모 타이머 정지
	void StopPassiveDrain();

	// 타이머 콜백: 배터리 차감 + 고갈 검사
	UFUNCTION()
	void OnPassiveDrainTick();

	// 배터리가 고갈되었을 때 서브클래스에서 override하여 처리 (예: 강제 꺼짐)
	virtual void OnBatteryDepleted();
};
