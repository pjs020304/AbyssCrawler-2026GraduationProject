// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LobbyGameMode.h"
#include "TitleGameInstance.h"
#include <MovieSceneSequencePlayer.h>

void ALobbyPlayerController::Server_HandleReadyButton_Implementation()
{
	if (HasAuthority() == false)
		return;

	ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(GetPawn()->GetPlayerState());
	if (LobbyPlayerState)
	{
		Client_SetSelectedPlayerColorIndex(LobbyPlayerState->PlayerColorIndex);

		LobbyPlayerState->Multicast_Ready();
	}

	ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(this));
	if (LobbyGameMode)
		LobbyGameMode->TryStartGame();

}

void ALobbyPlayerController::Server_HandleChangeUsername_Implementation(const FText& InNickname)
{
	if (HasAuthority() == false)
		return;

	ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(GetPawn()->GetPlayerState());
	if (LobbyPlayerState)
	{
		LobbyPlayerState->Nickname = InNickname;

		if (UTitleGameInstance* GI = GetGameInstance<UTitleGameInstance>())
		{
			const FString PlayerKey = PlayerState ? PlayerState->GetPlayerName() : GetName();
			GI->SavePlayerNickname(PlayerKey, InNickname);
		}
	}
}

void ALobbyPlayerController::HandleReadyButton()
{
	Server_HandleReadyButton();
}

void ALobbyPlayerController::HandleChangeNickname(const FText& InNickname)
{
	Server_HandleChangeUsername(InNickname);
}

void ALobbyPlayerController::Server_SetPlayerColorIndex_Implementation(int32 NewIndex)
{
	ALobbyPlayerState* LobbyPS = GetPlayerState<ALobbyPlayerState>();
	if (!LobbyPS)
	{
		return;
	}

	LobbyPS->SetPlayerColorIndex(NewIndex);

	Client_SetSelectedPlayerColorIndex(NewIndex);

	UE_LOG(LogTemp, Warning, TEXT("[LobbyColor] Server Set ColorIndex=%d"), NewIndex);
}

void ALobbyPlayerController::Client_SetSelectedPlayerColorIndex_Implementation(int32 NewIndex)
{
	if (UTitleGameInstance* GI = GetGameInstance<UTitleGameInstance>())
	{
		GI->SetSelectedPlayerColorIndex(NewIndex);

		UE_LOG(LogTemp, Warning, TEXT("[ColorGI] Client Saved ColorIndex=%d"), NewIndex);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ColorGI] TitleGameInstance is NULL"));
	}
}