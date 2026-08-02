// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleWidget.generated.h"

class UButton;
class UJoinPopupWidget;
class UPasswordPopupWidget;
class USoundBase;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API UTitleWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual bool Initialize() override;

protected:
	UFUNCTION()
	void OnClicked_BtnJoin();

	UFUNCTION()
	void OnClicked_BtnCreate();

	UFUNCTION()
	void OnClicked_BtnQuit();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Create;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title")
	TSubclassOf<UJoinPopupWidget> JoinPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title")
	TSubclassOf<UPasswordPopupWidget> PasswordPopupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> ClickSound;

};
