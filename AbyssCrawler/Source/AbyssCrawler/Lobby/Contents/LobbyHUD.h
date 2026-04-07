// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Lobby/UI/LobbyWidget.h"
#include "LobbyHUD.generated.h"

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API ALobbyHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	//UFUNCTION(BlueprintImplementableEvent)
	void RefreshUI();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class ULobbyWidget> LobbyWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class ULobbyWidget* LobbyUI;
	
};
