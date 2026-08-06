// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyUserWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "AbyssCrawler/Lobby/Contents/LobbyPlayerState.h"
#include "AbyssCrawler/Lobby/Contents/LobbyPlayerController.h"
#include "TitleGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

void ULobbyUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_ColorPrev)
	{
		Btn_ColorPrev->OnClicked.AddDynamic(this, &ULobbyUserWidget::HandlePrevColorClicked);
	}

	if (Btn_ColorNext)
	{
		Btn_ColorNext->OnClicked.AddDynamic(this, &ULobbyUserWidget::HandleNextColorClicked);
	}
}

void ULobbyUserWidget::SetInfo(ALobbyPlayerState* InPlayerState)
{
	if (PlayerState)
	{
		PlayerState->OnLobbyColorChanged.RemoveAll(this);
	}

	PlayerState = InPlayerState;

	if (PlayerState)
	{
		PlayerState->OnLobbyColorChanged.AddUObject(this, &ULobbyUserWidget::RefreshUI);
	}

	RefreshUI();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DelayedRefreshTimerHandle);

		World->GetTimerManager().SetTimer(
			DelayedRefreshTimerHandle,
			this,
			&ULobbyUserWidget::RefreshUI,
			0.2f,
			false
		);
	}
}

void ULobbyUserWidget::RefreshUI()
{
	if (PlayerState == nullptr)
	{
		return;
	}

	APlayerController* OwningPC = GetOwningPlayer();
	APlayerState* LocalPS = OwningPC ? OwningPC->PlayerState : nullptr;

	const bool bIsMine = LocalPS && LocalPS == PlayerState;

	/*
	if (bIsMine && PlayerState)
	{
		if (UTitleGameInstance* GI = GetGameInstance<UTitleGameInstance>())
		{
			GI->SetSelectedPlayerColorIndex(PlayerState->PlayerColorIndex);

			UE_LOG(LogTemp, Warning, TEXT("[ColorGI] Sync From Lobby UI ColorIndex=%d"),
				PlayerState->PlayerColorIndex);
		}
	}
	*/
	const bool bIsReady = PlayerState->Ready;
	const bool bIsServer = UKismetSystemLibrary::IsServer(this);

	// Hide UI
	Btn_Ready->SetVisibility(ESlateVisibility::Hidden);
	Btn_KickPlayer->SetVisibility(ESlateVisibility::Hidden);
	Txt_Ready->SetVisibility(ESlateVisibility::Hidden);
	Txt_PlayerName->SetVisibility(ESlateVisibility::Hidden);
	Editable_PlayerName->SetVisibility(ESlateVisibility::Hidden);

	if (Btn_ColorPrev)
	{
		Btn_ColorPrev->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Btn_ColorNext)
	{
		Btn_ColorNext->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Ready Text
	if (bIsReady)
	{
		Txt_Ready->SetVisibility(ESlateVisibility::Visible);
	}

	// Ready Button: 자기 슬롯에서만
	if (bIsMine && !bIsReady)
	{
		Btn_Ready->SetVisibility(ESlateVisibility::Visible);
	}

	// Kick Button: 서버가 다른 플레이어에게만
	if (bIsServer && !bIsMine)
	{
		Btn_KickPlayer->SetVisibility(ESlateVisibility::Visible);
	}

	// Nickname
	Editable_PlayerName->SetText(PlayerState->Nickname);
	Editable_PlayerName->SetVisibility(ESlateVisibility::Visible);
	Editable_PlayerName->SetIsReadOnly(!bIsMine || bIsReady);

	// Color Preview
	if (Img_ColorPreview)
	{
		Img_ColorPreview->SetColorAndOpacity(PlayerState->GetPlayerColor());
	}

	// 색상 변경 버튼: 자기 슬롯에서만
	if (Btn_ColorPrev)
	{
		Btn_ColorPrev->SetVisibility(bIsMine && !bIsReady ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (Btn_ColorNext)
	{
		Btn_ColorNext->SetVisibility(bIsMine && !bIsReady ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	UE_LOG(LogTemp, Warning, TEXT("[LobbyUserUI] Refresh Row=%s LocalPS=%s bIsMine=%d ColorIndex=%d"),
		PlayerState ? *PlayerState->GetName() : TEXT("NULL"),
		LocalPS ? *LocalPS->GetName() : TEXT("NULL"),
		bIsMine,
		PlayerState->PlayerColorIndex);

}

void ULobbyUserWidget::HandlePrevColorClicked()
{
	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC || OwningPC->PlayerState != PlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyColor] Prev ignored. Not my row."));
		return;
	}

	if (!PlayerState)
	{
		return;
	}

	const int32 MaxColorCount = 3;
	const int32 NewIndex = (PlayerState->PlayerColorIndex - 1 + MaxColorCount) % MaxColorCount;

	if (UTitleGameInstance* GI = GetGameInstance<UTitleGameInstance>())
	{
		GI->SetSelectedPlayerColorIndex(NewIndex);
	}

	if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(OwningPC))
	{
		LobbyPC->Server_SetPlayerColorIndex(NewIndex);
	}

	UE_LOG(LogTemp, Warning, TEXT("[LobbyColor] Prev Click NewIndex=%d"), NewIndex);
}

void ULobbyUserWidget::HandleNextColorClicked()
{
	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC || OwningPC->PlayerState != PlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyColor] Next ignored. Not my row."));
		return;
	}

	if (!PlayerState)
	{
		return;
	}

	const int32 MaxColorCount = 3;
	const int32 NewIndex = (PlayerState->PlayerColorIndex + 1) % MaxColorCount;

	if (UTitleGameInstance* GI = GetGameInstance<UTitleGameInstance>())
	{
		GI->SetSelectedPlayerColorIndex(NewIndex);
	}

	if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(OwningPC))
	{
		LobbyPC->Server_SetPlayerColorIndex(NewIndex);
	}

	UE_LOG(LogTemp, Warning, TEXT("[LobbyColor] Next Click NewIndex=%d"), NewIndex);
}