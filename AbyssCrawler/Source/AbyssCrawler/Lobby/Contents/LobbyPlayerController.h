// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_HandleReadyButton();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_HandleChangeUsername(const FText& InNickname);

	// UI 호출
	UFUNCTION(BlueprintCallable)
	void HandleReadyButton();

	UFUNCTION(BlueprintCallable)
	void HandleChangeNickname(const FText& InNickname);

	UFUNCTION(Server, Reliable)
	void Server_SetPlayerColorIndex(int32 NewIndex);
};
