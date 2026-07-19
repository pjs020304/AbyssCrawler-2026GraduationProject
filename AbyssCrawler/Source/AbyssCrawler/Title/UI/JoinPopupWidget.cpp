// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/UI/JoinPopupWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"

bool UJoinPopupWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (Btn_Cancel)
	{
		Btn_Cancel->OnClicked.AddDynamic(this, &UJoinPopupWidget::OnClicked_BtnCancel);
	}

	if (Btn_Join)
	{
		Btn_Join->OnClicked.AddDynamic(this, &UJoinPopupWidget::OnClicked_BtnJoin);
	}

	return true;
}

void UJoinPopupWidget::OnClicked_BtnCancel()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}

	RemoveFromParent();
}

void UJoinPopupWidget::OnClicked_BtnJoin()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	FString IPString;
	FString PasswordString;

	if (TxtBox_InputIP)
	{
		IPString = TxtBox_InputIP->GetText().ToString();
	}

	if (TxtBox_InputPassword)
	{
		PasswordString = TxtBox_InputPassword->GetText().ToString();
	}

	FString Command;

	if (PasswordString.IsEmpty())
	{
		Command = FString::Printf(TEXT("open %s"), *IPString);
	}
	else
	{
		Command = FString::Printf(TEXT("open %s?Password=%s"), *IPString, *PasswordString);
	}

	PC->ConsoleCommand(Command);

	if (ConnectingPopupClass)
	{
		ConnectingPopup = CreateWidget<UUserWidget>(GetWorld(), ConnectingPopupClass);
		if (ConnectingPopup)
		{
			ConnectingPopup->AddToViewport();

			if (GetWorld())
			{
				GetWorld()->GetTimerManager().SetTimer(
					ConnectingPopupTimerHandle,
					this,
					&UJoinPopupWidget::RemoveConnectingPopup,
					5.0f,
					false
				);
			}
		}
	}
}

void UJoinPopupWidget::RemoveConnectingPopup()
{
	if (ConnectingPopup)
	{
		ConnectingPopup->RemoveFromParent();
	}
}