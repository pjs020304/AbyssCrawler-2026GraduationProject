// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EndingGameMode.generated.h"

class ALevelSequenceActor;
class UUserWidget;

UCLASS()
class ABYSSCRAWLER_API AEndingGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AEndingGameMode();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Ending")
	float StartDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Ending")
	TSubclassOf<UUserWidget> EndingClearWidgetClass;

	FTimerHandle StartEndingTimerHandle;

	void StartEndingForPlayers();

	ALevelSequenceActor* FindEndingSequenceActor() const;

	virtual void PostLogin(APlayerController* NewPlayer) override;
};
