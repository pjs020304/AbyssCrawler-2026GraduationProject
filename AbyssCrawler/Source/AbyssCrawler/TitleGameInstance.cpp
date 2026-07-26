// Fill out your copyright notice in the Description page of Project Settings.


#include "TitleGameInstance.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Misc/ConfigCacheIni.h"
#include "GameFramework/GameUserSettings.h"
#include "LoadingScreenSubsystem.h"
#include "LoadingScreenWidget.h"

void UTitleGameInstance::Init()
{
	Super::Init();

	ApplySavedGameConfig();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UTitleGameInstance::OnNetworkFailure);
	}

	if (ULoadingScreenSubsystem* LoadingSubsystem = GetSubsystem<ULoadingScreenSubsystem>())
	{
		LoadingSubsystem->SetLoadingWidgetClass(LoadingScreenWidgetClass);

		UE_LOG(LogTemp, Warning, TEXT("[Loading] GameInstance Set WidgetClass: %s"),
			LoadingScreenWidgetClass ? *LoadingScreenWidgetClass->GetName() : TEXT("NULL"));
	}
}

void UTitleGameInstance::OnNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString
)
{
	UE_LOG(LogTemp, Error, TEXT("[OnNetworkFailure] FailureType = %d, ErrorString = %s"), (int32)FailureType, *ErrorString);

	// 1. WrongPassword 아니면 무시
	if (!ErrorString.Contains(TEXT("WrongPassword")))
	{
		return;
	}

	// 2. 접속 시도 실패가 아니면 무시
	if (FailureType != ENetworkFailure::PendingConnectionFailure)
	{
		return;
	}

	// 3. 로컬 플레이어가 없으면 무시
	ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("[OnNetworkFailure] LocalPlayer is NULL"));
		return;
	}

	// 4. 현재 로컬 PC가 없으면 무시
	APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld());
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[OnNetworkFailure] PlayerController is NULL"));
		return;
	}

	// 5. 타이틀 맵이 아니면 무시 (중요)
	UWorld* CurrentWorld = PC->GetWorld();
	if (!CurrentWorld)
	{
		return;
	}

	const FString CurrentMapName = CurrentWorld->GetMapName();
	if (!CurrentMapName.Contains(TEXT("Title")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnNetworkFailure] Not in Title map, ignore popup"));
		return;
	}

	/*
	// 6. 팝업 띄우기
	if (!WrongPopupClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[OnNetworkFailure] WrongPopupClass is NULL"));
		return;
	}

	UUserWidget* Popup = CreateWidget<UUserWidget>(PC, WrongPopupClass);
	if (!Popup)
	{
		UE_LOG(LogTemp, Error, TEXT("[OnNetworkFailure] Popup Create Failed"));
		return;
	}

	Popup->AddToViewport(100);
	*/
}

void UTitleGameInstance::SavePlayerNickname(const FString& PlayerKey, const FText& Nickname)
{
	PlayerNicknames.Add(PlayerKey, Nickname);
}

FText UTitleGameInstance::GetSavedNickname(const FString& PlayerKey) const
{
	if (const FText* FoundName = PlayerNicknames.Find(PlayerKey))
	{
		return *FoundName;
	}

	return FText::FromString(TEXT("Player"));
}

void UTitleGameInstance::ApplySavedGameConfig()
{
	float SavedVolume = 1.0f;

	GConfig->GetFloat(
		TEXT("/Script/AbyssCrawler.GameConfig"),
		TEXT("MasterVolume"),
		SavedVolume,
		GGameUserSettingsIni
	);

	SavedVolume = FMath::Clamp(SavedVolume, 0.0f, 1.0f);

	if (SettingSoundMix && MasterSoundClass)
	{
		UGameplayStatics::PushSoundMixModifier(this, SettingSoundMix);

		UGameplayStatics::SetSoundMixClassOverride(
			this,
			SettingSoundMix,
			MasterSoundClass,
			SavedVolume,
			1.0f,
			0.0f,
			true
		);
	}

	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->ApplySettings(false);
	}
}

void UTitleGameInstance::SaveAndApplyVolume(float Volume)
{
	Volume = FMath::Clamp(Volume, 0.0f, 1.0f);

	GConfig->SetFloat(
		TEXT("/Script/AbyssCrawler.GameConfig"),
		TEXT("MasterVolume"),
		Volume,
		GGameUserSettingsIni
	);

	GConfig->Flush(false, GGameUserSettingsIni);

	if (SettingSoundMix && MasterSoundClass)
	{
		UGameplayStatics::PushSoundMixModifier(this, SettingSoundMix);

		UGameplayStatics::SetSoundMixClassOverride(
			this,
			SettingSoundMix,
			MasterSoundClass,
			Volume,
			1.0f,
			0.0f,
			true
		);
	}
}
