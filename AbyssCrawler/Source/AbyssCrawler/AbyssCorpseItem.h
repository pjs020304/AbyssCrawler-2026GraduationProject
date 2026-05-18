// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h"
#include "AbyssCorpseItem.generated.h"

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API AAbyssCorpseItem : public AAbyssItemBase
{
	GENERATED_BODY()

public:
	AAbyssCorpseItem();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse")
	int32 SlotCost = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse")
	float CarrySpeedMultiplier = 0.5f;

	void SetDeadPlayerState(APlayerState* InPlayerState);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated)
	APlayerState* DeadPlayerState;
	
};
