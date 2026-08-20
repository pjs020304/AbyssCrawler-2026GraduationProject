// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class UMissionUIWidget;
class UChatting;

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

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_OpenChat();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_CloseChat();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_ToggleMission();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_CloseMission();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_OpenMission();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_ShowGameClear();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_ShowGameOver();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_PlayFadeOut();

	// Intensity: 0 = 경고 범위 진입, 1 = 벽에 밀착
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_ShowBoundaryWarning(float Intensity);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_HideBoundaryWarning();

	UFUNCTION(BlueprintCallable)
	UChatting* GetChattingUI() const;

	void AddChatMessage(const FString& Message);
	TSharedPtr<SWidget> GetChatInputTextObject();

protected:
	UPROPERTY(meta = (BindWidget))
	UMissionUIWidget* MissionUI;

	// 위젯이 초기화될 때 호출
	virtual void NativeConstruct() override;

	// 보유 금액이 바뀔 때 호출되는 UI 갱신 함수
	UFUNCTION()
	void UpdateMoneyDisplay(int32 NewMoney);

	// 블루프린트에서 텍스트를 바꿀 수 있도록 노출하는 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUpdateMoneyText(int32 Money);
	
	UPROPERTY(meta = (BindWidgetOptional))
	UChatting* ChattingUI;

	UFUNCTION()
	void UpdateRemainingTimeDisplay(float NewTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUpdateRemainingTimeText(const FText& TimeText);
};
