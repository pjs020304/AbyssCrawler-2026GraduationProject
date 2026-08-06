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

	// ������ �ʱ�ȭ�� �� ȣ��
	virtual void NativeConstruct() override;

	// ���� ������ �� ����� UI ������Ʈ �Լ�
	UFUNCTION()
	void UpdateMoneyDisplay(int32 NewMoney);

	// ��������Ʈ���� �ؽ�Ʈ�� �ٲ� �� �ֵ��� �̺�Ʈ ����
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUpdateMoneyText(int32 Money);
	
	UPROPERTY(meta = (BindWidgetOptional))
	UChatting* ChattingUI;

	UFUNCTION()
	void UpdateRemainingTimeDisplay(float NewTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUpdateRemainingTimeText(const FText& TimeText);
};
