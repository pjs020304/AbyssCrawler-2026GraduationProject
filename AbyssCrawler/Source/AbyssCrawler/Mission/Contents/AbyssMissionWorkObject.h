// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssDiverCharacter.h"
#include "AbyssMissionWorkObject.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;
class AAbyssSharkCharacter;
class UMissionInteractPromptWidget;

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionWorkObject : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionWorkObject();

	virtual void Interact_Implementation(AActor* InstigatorActor) override;

	virtual void Tick(float DeltaTime) override;

	void CancelWork();

	float CurrentWorkTime = 0.0f;

	// 미니맵이 미션 목표 위치를 수집할 때 사용 (AAbyssMissionArea::GetMissionId와 동일한 관례)
	UFUNCTION(BlueprintPure, Category = "Mission")
	FName GetMissionId() const { return MissionId; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 MissionIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	float WorkDuration = 5.0f;

	UPROPERTY()
	AAbyssDiverCharacter* WorkingCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName MissionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	float EnemyCancelRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FText WorkDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FText WorkDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bCancelByEnemyNearby = true;

	FTimerHandle WorkTimerHandle;

	bool bIsWorking = false;
	bool bCompleted = false;

	void CompleteWork();

	bool IsEnemyNearWorkingCharacter() const;

	// InterectPrompt
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> MissionPromptWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText PromptObjectName = FText::FromString(TEXT("Mission Object"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText ActiveStateText = FText::FromString(TEXT("Worked E"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText InactiveStateText = FText::FromString(TEXT("Mission Not Accepted"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText CompletedStateText = FText::FromString(TEXT("Completed"));

	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

	void UpdateMissionPromptUI();
	bool IsMissionActiveForPrompt() const;

};
