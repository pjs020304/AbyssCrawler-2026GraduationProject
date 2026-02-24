// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssItemBase.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssItemBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAbyssItemBase();
	
	virtual void UseItem();

	UPROPERTY(EditDefaultsonly, Category = "Item Info")
	FString ItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* ItemIcon;

};
