// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class UMissionUIWidget;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void RefreshMissionUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_RefreshInventoryUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_UpdateHealthUI(float CurrentValue, float MaxValue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_UpdateOxygenUI(float CurrentValue, float MaxValue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_UpdateBatteryUI(float CurrentValue, float MaxValue);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_ShowWorkUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_HideWorkUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_UpdateWorkProgress(float Progress);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_ShowMissionComplete(const FText& MissionName);

protected:
	UPROPERTY(meta = (BindWidget))
	UMissionUIWidget* MissionUI;
	
};
