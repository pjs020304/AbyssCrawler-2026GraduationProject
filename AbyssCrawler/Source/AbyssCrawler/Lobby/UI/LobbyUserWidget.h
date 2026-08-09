// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "LobbyUserWidget.generated.h"

class UTextBlock;
class UEditableTextBox;
class UButton;
class ALobbyPlayerState;
class UImage;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API ULobbyUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void SetInfo(ALobbyPlayerState* InPlayerState);

	UFUNCTION(BlueprintCallable)
	void RefreshUI();


protected:

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Ready;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_PlayerName;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Editable_PlayerName;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Ready;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Btn_KickPlayer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_ColorPrev;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_ColorNext;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_ColorPreview;

	UFUNCTION()
	void HandlePrevColorClicked();

	UFUNCTION()
	void HandleNextColorClicked();

protected:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<ALobbyPlayerState> PlayerState;

protected:
	FTimerHandle DelayedRefreshTimerHandle;
};
