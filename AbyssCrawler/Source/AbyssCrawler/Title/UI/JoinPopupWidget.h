// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JoinPopupWidget.generated.h"

class UButton;
class UEditableTextBox;
class UUserWidget;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API UJoinPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	UFUNCTION()
	void OnClicked_BtnCancel();

	UFUNCTION()
	void OnClicked_BtnJoin();

	UFUNCTION()
	void RemoveConnectingPopup();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TxtBox_InputIP;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TxtBox_InputPassword;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Cancel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JoinPopup")
	TSubclassOf<UUserWidget> ConnectingPopupClass;

	UPROPERTY(BlueprintReadWrite, Category = "JoinPopup")
	TObjectPtr<UUserWidget> ConnectingPopup;

	FTimerHandle ConnectingPopupTimerHandle;
	
};
