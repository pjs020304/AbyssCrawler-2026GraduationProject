// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/UI/TitleWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "JoinPopupWidget.h"
#include "PasswordPopupWidget.h"

bool UTitleWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (Btn_Join)
	{
		Btn_Join->OnClicked.AddDynamic(this, &UTitleWidget::OnClicked_BtnJoin);
	}

	if (Btn_Create)
	{
		Btn_Create->OnClicked.AddDynamic(this, &UTitleWidget::OnClicked_BtnCreate);
	}

	return true;
}

void UTitleWidget::OnClicked_BtnJoin()
{
	if (!JoinPopupClass)
	{
		return;
	}

	UJoinPopupWidget* JoinPopup = CreateWidget<UJoinPopupWidget>(GetWorld(), JoinPopupClass);
	if (JoinPopup)
	{
		JoinPopup->AddToViewport();
	}
}

void UTitleWidget::OnClicked_BtnCreate()
{
	//UGameplayStatics::OpenLevel(this, FName(TEXT("Lobby")), true, TEXT("listen"));

	if (!PasswordPopupClass)
	{
		return;
	}

	UPasswordPopupWidget* PasswordPopup = CreateWidget<UPasswordPopupWidget>(GetWorld(), PasswordPopupClass);
	if (PasswordPopup)
	{
		PasswordPopup->AddToViewport();
	}
}