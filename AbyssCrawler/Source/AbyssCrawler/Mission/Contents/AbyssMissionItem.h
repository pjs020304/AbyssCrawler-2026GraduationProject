// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssMissionItem.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;
class UMissionInteractPromptWidget;

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionItem : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionItem();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 MissionIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FText ItemDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bDestroyOnCollect = true;

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	// Mission Prompt
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> MissionPromptWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText PromptObjectName = FText::FromString(TEXT("Mission Item"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText ActiveStateText = FText::FromString(TEXT("Collect"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText InactiveStateText = FText::FromString(TEXT("Mission Not Accepted"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FVector PromptWidgetOffset = FVector(0.0f, 0.0f, 40.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> PickupSound;

public:	
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

protected:
	void UpdateMissionPromptUI();
	bool IsMissionActiveForPrompt() const;
};
