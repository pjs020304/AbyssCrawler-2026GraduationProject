// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/UI/ChatMessage.h"
#include "Components/TextBlock.h"

void UChatMessage::SetChatMessage(const FString& Message)
{
	if (!ChatMessageTextBlock) return;

	ChatMessageTextBlock->SetText(FText::FromString(Message));
}

