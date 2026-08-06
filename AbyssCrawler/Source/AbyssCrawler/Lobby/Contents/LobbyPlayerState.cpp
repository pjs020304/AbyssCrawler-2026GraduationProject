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
	DOREPLIFETIME(ALobbyPlayerState, PlayerColorIndex);
}

void ALobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	UE_LOG(LogTemp, Warning, TEXT("[ColorCopy] CopyProperties Called. From LobbyPS=%s ColorIndex=%d TargetPS=%s"),
		*GetName(),
		PlayerColorIndex,
		PlayerState ? *PlayerState->GetName() : TEXT("NULL"));

	AAbyssPlayerState* NewPlayerState = Cast<AAbyssPlayerState>(PlayerState);

	if (NewPlayerState)
	{
		NewPlayerState->Nickname = Nickname;
	}

	NewPlayerState->Nickname = Nickname;
	NewPlayerState->PlayerColorIndex = PlayerColorIndex;

	UE_LOG(LogTemp, Warning, TEXT("[ColorCopy] Copied ColorIndex=%d to AbyssPS=%s"),
		PlayerColorIndex,
		*NewPlayerState->GetName());
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

void ALobbyPlayerState::AddPlayerColorIndex(int32 Delta)
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 MaxColorCount = 3;

	PlayerColorIndex = (PlayerColorIndex + Delta + MaxColorCount) % MaxColorCount;

	OnRep_PlayerColorIndex();

	UE_LOG(LogTemp, Warning, TEXT("[LobbyColor] Changed ColorIndex=%d"), PlayerColorIndex);
}

FLinearColor ALobbyPlayerState::GetPlayerColor() const
{
	switch (PlayerColorIndex)
	{
	case 0:
		return FLinearColor(0.0f, 0.6f, 0.1f, 1.0f); // Green

	case 1:
		return FLinearColor(0.8f, 0.0f, 0.0f, 1.0f); // Red

	case 2:
		return FLinearColor(0.0f, 0.2f, 1.0f, 1.0f); // Blue

	default:
		return FLinearColor::White;
	}
}

void ALobbyPlayerState::SetPlayerColorIndex(int32 NewIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 MaxColorCount = 3;
	PlayerColorIndex = FMath::Clamp(NewIndex, 0, MaxColorCount - 1);

	OnRep_PlayerColorIndex();
}

void ALobbyPlayerState::OnRep_PlayerColorIndex()
{
	UE_LOG(LogTemp, Warning, TEXT("[LobbyColor] OnRep ColorIndex=%d"), PlayerColorIndex);

	OnLobbyColorChanged.Broadcast();
}
