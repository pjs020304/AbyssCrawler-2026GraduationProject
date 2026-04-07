// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/Contents/TitlePlayerController.h"
#include "Title/UI/TitleWidget.h"

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (TitleWidgetClass && IsLocalController())
	{
		TitleWidget = CreateWidget<UTitleWidget>(this, TitleWidgetClass);
		if (TitleWidget)
		{
			TitleWidget->AddToViewport();

			FInputModeGameAndUI InputMode;
			//InputMode.SetWidgetToFocus(TitleWidget->TakeWidget());
			SetInputMode(InputMode);

			bShowMouseCursor = true;
		}
	}
}