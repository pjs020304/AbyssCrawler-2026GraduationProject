// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextWidgetTypes.h"
#include "Chatting.generated.h"

class UScrollBox;
class UEditableTextBox;
class UButton;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API UChatting : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* ChatScrollBox;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* ChattingBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chat")
	TSubclassOf<class UChatMessage> ChatMessageClass;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_CloseChat;

	UFUNCTION(BlueprintCallable)
	void AddChatMessage(const FString& Message);

	TSharedPtr<class SWidget> GetChatInputTextObject();

	UFUNCTION(BlueprintCallable)
	void FocusChatInput();

	UFUNCTION()
	void HandleCloseChatClicked();

private:
	UFUNCTION()
	void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
};
