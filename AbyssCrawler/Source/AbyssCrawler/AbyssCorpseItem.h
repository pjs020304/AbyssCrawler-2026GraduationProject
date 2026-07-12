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

	// 이 시체의 주인 (부활 장치 등에서 시체 정리 판단용)
	APlayerState* GetDeadPlayerState() const { return DeadPlayerState; }

	void InitCorpse(USkeletalMesh* DiverMesh, TArray<UMaterialInterface*> Materials);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* CorpseMesh;

protected:
	UPROPERTY(Replicated)
	APlayerState* DeadPlayerState;
	
};
