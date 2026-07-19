// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionInteractPromptWidget.generated.h"

class UTextBlock;

UCLASS()
class ABYSSCRAWLER_API UMissionInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void UpdateMissionPrompt(
		const FText& ObjectName,
		const FText& StateText,
		bool bCanInteract
	);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Key;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_ObjectName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_State;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prompt")
	FLinearColor CanInteractColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prompt")
	FLinearColor CannotInteractColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prompt")
	FLinearColor KeyColor = FLinearColor::Yellow;
};
