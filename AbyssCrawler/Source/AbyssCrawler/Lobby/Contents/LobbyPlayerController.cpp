// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LobbyGameMode.h"
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

void ALobbyPlayerController::Server_HandleChangeUsername_Implementation(const FText& InNIckname)
{
	if (HasAuthority() == false)
		return;

	ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(GetPawn()->GetPlayerState());
	if (LobbyPlayerState)
		LobbyPlayerState->Nickname = InNIckname;

}

void ALobbyPlayerController::HandleReadyButton()
{
	Server_HandleReadyButton();
}

void ALobbyPlayerController::HandleChangeNickname(const FText& InNickname)
{
	Server_HandleChangeUsername(InNickname);
}