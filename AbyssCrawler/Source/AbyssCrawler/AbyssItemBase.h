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

	// 슬롯에서 해제될 때 캐릭터에서 호출 (예: 슬롯 전환, 드롭)
	virtual void NotifyUnequipped();

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
