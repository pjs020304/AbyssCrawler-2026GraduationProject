// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h" 
#include "GameFramework/PlayerController.h"
#include "GameFramework/OnlineReplStructs.h"
#include "LobbyPlayerState.h"

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	bUseSeamlessTravel = true;
}

bool ALobbyGameMode::IsEverybodyReady()
{
	AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS)
		return false;

	for (APlayerState* PS : GS->PlayerArray)
	{
		ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);

		if (!LobbyPS)
			continue;

		if (!LobbyPS->Ready)
		{
			return false;
		}
	}

	return true;
}

void ALobbyGameMode::TryStartGame()
{
	if (IsEverybodyReady())
	{
		/*
		// Append
		FString Commnad = TEXT("ServerTravel L_DeepSea_VerLightStudio");

		// Execute Console Command
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->ConsoleCommand(Commnad);
		}
		*/

		GetWorld()->ServerTravel(TEXT("/Game/AbyssCrawler/Maps/MainGame/L_DeepSea_VerLightStudio"));
	}
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer) return;

	APlayerState* PS = NewPlayer->PlayerState;
	ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);

	if (!LobbyPS) return;

	NicknameIndex++;

	// Format Text
	FString NameString = FString::Printf(TEXT("User%d"), NicknameIndex);

	// Set Nickname
	LobbyPS->Nickname = FText::FromString(NameString);
}

void ALobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	ServerPassword = UGameplayStatics::ParseOption(Options, TEXT("Password"));

	UE_LOG(LogTemp, Warning, TEXT("[InitGame] ServerPassword = %s"), *ServerPassword);
}

void ALobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// 클라이언트가 보낸 Password 추출
	const FString InputPassword = UGameplayStatics::ParseOption(Options, TEXT("Password"));

	// 서버에 설정된 Password 검사
	UE_LOG(LogTemp, Warning, TEXT("[PreLogin] InputPassword = %s, ServerPassword = %s"), *InputPassword, *ServerPassword);

	if (!ServerPassword.IsEmpty() && InputPassword != ServerPassword)
	{
		ErrorMessage = TEXT("WrongPassword");
		UE_LOG(LogTemp, Error, TEXT("[PreLogin] WrongPassword"));
	}
}


