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
	
	virtual void UseItem();
	virtual void EndUseItem();

	UPROPERTY(EditDefaultsonly, BlueprintReadOnly, Category = "Item Info")
	FString ItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* ItemIcon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category =  "Item Info")
	int32 ItemPrice;

	virtual void Interact_Implementation(AActor* InstigatorActor) override;

	// 3D 허공에 띄울 UI 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* InteractWidgetComp;

	// 인터페이스 오버라이드
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

	// 아이템 상태 전환
	void SetAsPickedUp(AAbyssDiverCharacter* NewOwnerCharacter, USceneComponent* AttachParent, bool bVisibleInHand);
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

	// 이 아이템을 사용할 때 소모될 배터리 양
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Cost")
	float BatteryCost;

	// 배터리를 깎을 공용 이펙트 클래스 (GE_ConsumeBattery)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Cost")
	TSubclassOf<class UGameplayEffect> BatteryConsumeEffectClass;

	// SetByCaller로 값을 넘겨주기 위한 연결 고리 태그 (예: "Data.Cost.Battery")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Cost")
	FGameplayTag BatteryCostTag;
};
