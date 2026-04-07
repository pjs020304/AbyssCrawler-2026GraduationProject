// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleWidget.generated.h"

class UButton;
class UJoinPopupWidget;
class UPasswordPopupWidget;

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

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Create;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title")
	TSubclassOf<UJoinPopupWidget> JoinPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title")
	TSubclassOf<UPasswordPopupWidget> PasswordPopupClass;

};
