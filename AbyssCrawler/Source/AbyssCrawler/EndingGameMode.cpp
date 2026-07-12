// Fill out your copyright notice in the Description page of Project Settings.


#include "EndingGameMode.h"

#include "AbyssPlayerController.h"
#include "EngineUtils.h"
#include "LevelSequenceActor.h"

AEndingGameMode::AEndingGameMode()
{
	PlayerControllerClass = AAbyssPlayerController::StaticClass();

	// 엔딩 맵에서는 플레이어 캐릭터를 굳이 스폰하지 않으려면 nullptr
	DefaultPawnClass = nullptr;
}

void AEndingGameMode::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		StartEndingTimerHandle,
		this,
		&AEndingGameMode::StartEndingForPlayers,
		1.0f,
		false
	);
}

ALevelSequenceActor* AEndingGameMode::FindEndingSequenceActor() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ALevelSequenceActor> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

void AEndingGameMode::StartEndingForPlayers()
{
	UE_LOG(LogTemp, Warning, TEXT("[Ending] StartEndingForPlayers"));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AAbyssPlayerController* PC = Cast<AAbyssPlayerController>(It->Get()))
		{
			PC->Client_StartEndingSequence(EndingClearWidgetClass);
		}
	}
}

void AEndingGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	FTimerHandle TimerHandle;

	GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, NewPlayer]()
			{
				if (AAbyssPlayerController* PC = Cast<AAbyssPlayerController>(NewPlayer))
				{
					PC->Client_StartEndingSequence(EndingClearWidgetClass);
				}
			}),
		1.0f,
		false
	);
}