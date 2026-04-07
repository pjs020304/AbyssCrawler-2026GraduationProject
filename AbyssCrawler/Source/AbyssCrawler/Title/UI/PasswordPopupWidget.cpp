// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/UI/PasswordPopupWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"

bool UPasswordPopupWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (Btn_Cancel)
	{
		Btn_Cancel->OnClicked.AddDynamic(this, &UPasswordPopupWidget::OnClicked_BtnCancel);
	}

	if (Btn_Create)
	{
		Btn_Create->OnClicked.AddDynamic(this, &UPasswordPopupWidget::OnClicked_BtnCreate);
	}

	return true;
}

void UPasswordPopupWidget::OnClicked_BtnCancel()
{
	RemoveFromParent();
}

void UPasswordPopupWidget::OnClicked_BtnCreate()
{
	FString PasswordString;

	if (TxtBox_InputPassword)
	{
		PasswordString = TxtBox_InputPassword->GetText().ToString();
	}

	FString Options = TEXT("listen");

	if (!PasswordString.IsEmpty())
	{
		Options += FString::Printf(TEXT("?Password=%s"), *PasswordString);
	}

	UE_LOG(LogTemp, Warning, TEXT("[CreateRoom] Options = %s"), *Options);

	UGameplayStatics::OpenLevel(this, FName(TEXT("Lobby")), true, Options);
}
