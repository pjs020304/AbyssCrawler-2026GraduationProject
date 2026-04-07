// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "LobbyHUD.h"
#include "AbyssPlayerState.h"

void ALobbyPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Delay Untill Next Tick
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ALobbyPlayerState::RefreshLobbyUI);

	// OnDestroyed Event
	OnDestroyed.AddDynamic(this, &ALobbyPlayerState::OnDestroyed_Event);
}

void ALobbyPlayerState::OnDestroyed_Event(AActor* DestroyedActor)
{
	RefreshLobbyUI();
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, Ready);
	DOREPLIFETIME(ALobbyPlayerState, Nickname);
}

void ALobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	AAbyssPlayerState* NewPlayerState = Cast<AAbyssPlayerState>(PlayerState);

	if (NewPlayerState)
	{
		NewPlayerState->Nickname = Nickname;
	}
}

void ALobbyPlayerState::Client_KickedByServer_Implementation()
{
	UGameplayStatics::OpenLevel(this, FName("Title"));
}

void ALobbyPlayerState::Multicast_Ready_Implementation()
{
	Ready = true;

	RefreshLobbyUI();
}

void ALobbyPlayerState::BP_RefreshLobbyUI()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	if (!PC) return;

	AHUD* HUD = PC->GetHUD();

	if (!HUD) return;

	ALobbyHUD* LobbyHUD = Cast<ALobbyHUD>(HUD);

	if (LobbyHUD)
	{
		LobbyHUD->RefreshUI();
	}

}

void ALobbyPlayerState::RefreshLobbyUI()
{
	BP_RefreshLobbyUI();
}

void ALobbyPlayerState::OnRep_NicknameChange()
{
	RefreshLobbyUI();
}

