// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Contents/LobbyHUD.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void ALobbyHUD::BeginPlay()
{
	Super::BeginPlay();

	if (LobbyWidgetClass)
	{
		LobbyUI = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);

		if (LobbyUI)
		{
			LobbyUI->AddToViewport();

			APlayerController* PC = GetOwningPlayerController();

			if (PC)
			{
				FInputModeGameAndUI InputMode;
				InputMode.SetWidgetToFocus(LobbyUI->TakeWidget());

				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
			}
		}
	}
}

void ALobbyHUD::RefreshUI()
{
	if (LobbyUI)
	{
		// BP 위젯 함수 호출
		LobbyUI->RefreshUI();
	}
}