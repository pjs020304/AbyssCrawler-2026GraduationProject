// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/UI/MissionInteractPromptWidget.h"
#include "Components/TextBlock.h"

void UMissionInteractPromptWidget::UpdateMissionPrompt(
	const FText& ObjectName,
	const FText& StateText,
	bool bCanInteract
)
{
	if (Txt_Key)
	{
		Txt_Key->SetText(FText::FromString(TEXT("[E]")));
		Txt_Key->SetColorAndOpacity(
			bCanInteract ? FSlateColor(KeyColor) : FSlateColor(CannotInteractColor)
		);
		Txt_Key->SetVisibility(bCanInteract ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (Txt_ObjectName)
	{
		Txt_ObjectName->SetText(ObjectName);
		Txt_ObjectName->SetColorAndOpacity(
			bCanInteract ? FSlateColor(CanInteractColor) : FSlateColor(CannotInteractColor)
		);
	}

	if (Txt_State)
	{
		Txt_State->SetText(StateText);
		Txt_State->SetColorAndOpacity(
			bCanInteract ? FSlateColor(CanInteractColor) : FSlateColor(CannotInteractColor)
		);
	}
}