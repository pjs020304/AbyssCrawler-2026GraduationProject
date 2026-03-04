// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "Components/WidgetComponent.h"
#include "AbyssItemBase.generated.h"

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
};
