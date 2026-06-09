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
		LobbyPlayerState->Multicast_Ready();

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