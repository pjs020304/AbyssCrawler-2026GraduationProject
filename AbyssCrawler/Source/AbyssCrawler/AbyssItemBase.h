// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
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
};
