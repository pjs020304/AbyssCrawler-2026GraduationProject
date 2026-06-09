// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/UI/Chatting.h"
#include "Chat/UI/ChatMessage.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "AbyssDiverCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MainHUDWidget.h"

void UChatting::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChattingBox)
	{
		ChattingBox->OnTextCommitted.AddDynamic(this, &UChatting::OnChatTextCommitted);
	}

	if (Btn_CloseChat)
	{
		Btn_CloseChat->OnClicked.AddDynamic(this, &UChatting::HandleCloseChatClicked);
	}
}

void UChatting::AddChatMessage(const FString& Message)
{
	if (!ChatScrollBox || !ChatMessageClass) return;

	UChatMessage* NewMessageWidget = CreateWidget<UChatMessage>(GetWorld(), ChatMessageClass);
	if (!NewMessageWidget) return;

	NewMessageWidget->SetChatMessage(Message);
	ChatScrollBox->AddChild(NewMessageWidget);
	ChatScrollBox->ScrollToEnd();
}

TSharedPtr<SWidget> UChatting::GetChatInputTextObject()
{
	return ChattingBox ? ChattingBox->GetCachedWidget() : nullptr;
}

void UChatting::FocusChatInput()
{
	if (ChattingBox)
	{
		ChattingBox->SetKeyboardFocus();
	}
}

void UChatting::HandleCloseChatClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(PC->GetPawn());
	if (!Diver) return;

	if (Diver->MainHUDRef)
	{
		Diver->MainHUDRef->BP_CloseChat();
	}

	Diver->SetInputLockedByUI(false);

	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = false;
}

void UChatting::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(PC->GetPawn());
	if (!Diver) return;

	switch (CommitMethod)
	{
	case ETextCommit::OnEnter:
	{
		const FString Message = Text.ToString().TrimStartAndEnd();

		if (!Message.IsEmpty())
		{
			Diver->SendChatMessage(Message);

			if (ChattingBox)
			{
				ChattingBox->SetText(FText::GetEmpty());
				ChattingBox->SetKeyboardFocus();
			}
		}

		break;
	}

	case ETextCommit::OnCleared:
	{
		break;
	}

	default:
		break;
	}
}
