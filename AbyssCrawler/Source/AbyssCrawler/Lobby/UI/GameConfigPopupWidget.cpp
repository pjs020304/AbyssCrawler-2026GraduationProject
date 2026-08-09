// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/UI/GameConfigPopupWidget.h"

#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundBase.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"
#include "TitleGameInstance.h"

void UGameConfigPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SettingSoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(this, SettingSoundMix);
	}

	float SavedVolume = 1.0f;
	GConfig->GetFloat(
		TEXT("/Script/AbyssCrawler.GameConfig"),
		TEXT("MasterVolume"),
		SavedVolume,
		GGameUserSettingsIni
	);

	SavedVolume = FMath::Clamp(SavedVolume, 0.0f, 1.0f);

	if (Slider_Volume)
	{
		Slider_Volume->SetValue(SavedVolume);
		Slider_Volume->OnValueChanged.AddDynamic(this, &UGameConfigPopupWidget::OnVolumeChanged);
	}

	ApplyVolume(SavedVolume);

	if (Check_Low)
	{
		Check_Low->OnCheckStateChanged.AddDynamic(this, &UGameConfigPopupWidget::OnLowChecked);
	}

	if (Check_Medium)
	{
		Check_Medium->OnCheckStateChanged.AddDynamic(this, &UGameConfigPopupWidget::OnMediumChecked);
	}

	if (Check_High)
	{
		Check_High->OnCheckStateChanged.AddDynamic(this, &UGameConfigPopupWidget::OnHighChecked);
	}

	if (Btn_Low)
	{
		Btn_Low->OnClicked.AddDynamic(this, &UGameConfigPopupWidget::OnLowClicked);
	}

	if (Btn_Medium)
	{
		Btn_Medium->OnClicked.AddDynamic(this, &UGameConfigPopupWidget::OnMediumClicked);
	}

	if (Btn_High)
	{
		Btn_High->OnClicked.AddDynamic(this, &UGameConfigPopupWidget::OnHighClicked);
	}

	if (Btn_Close)
	{
		Btn_Close->OnClicked.AddDynamic(this, &UGameConfigPopupWidget::OnCloseClicked);
	}

	if (Slider_Volume)
	{
		Slider_Volume->SetValue(SavedVolume);
		Slider_Volume->OnValueChanged.AddDynamic(this, &UGameConfigPopupWidget::OnVolumeChanged);
	}

	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		int32 CurrentQuality = Settings->GetOverallScalabilityLevel();

		if (CurrentQuality < 0)
		{
			CurrentQuality = 1;
		}

		// 지금 UI가 Low / Medium / High 3개라서 0~2로 사용
		CurrentQuality = FMath::Clamp(CurrentQuality, 0, 2);

		RefreshQualityChecks(CurrentQuality);
		RefreshVolumeUI(SavedVolume);
		ApplyVolume(SavedVolume);
	}
}

void UGameConfigPopupWidget::OnVolumeChanged(float Value)
{
	Value = FMath::Clamp(Value, 0.0f, 1.0f);

	RefreshVolumeUI(Value);

	if (UTitleGameInstance* GI = GetGameInstance<UTitleGameInstance>())
	{
		GI->SaveAndApplyVolume(Value);
	}
}

void UGameConfigPopupWidget::ApplyVolume(float Value)
{
	if (!SettingSoundMix || !MasterSoundClass)
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(
		this,
		SettingSoundMix,
		MasterSoundClass,
		Value,
		1.0f,
		0.0f,
		true
	);
}

void UGameConfigPopupWidget::OnLowChecked(bool bIsChecked)
{
	if (bUpdatingQualityCheck || !bIsChecked)
	{
		return;
	}

	ApplyQuality(0);
}

void UGameConfigPopupWidget::OnMediumChecked(bool bIsChecked)
{
	if (bUpdatingQualityCheck || !bIsChecked)
	{
		return;
	}

	ApplyQuality(1);
}

void UGameConfigPopupWidget::OnHighChecked(bool bIsChecked)
{
	if (bUpdatingQualityCheck || !bIsChecked)
	{
		return;
	}

	ApplyQuality(2);
}

void UGameConfigPopupWidget::ApplyQuality(int32 QualityLevel)
{
	QualityLevel = FMath::Clamp(QualityLevel, 0, 2);

	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SetOverallScalabilityLevel(QualityLevel);
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}

	RefreshQualityChecks(QualityLevel);
}

void UGameConfigPopupWidget::RefreshQualityChecks(int32 QualityLevel)
{
	bUpdatingQualityCheck = true;

	if (Check_Low)
	{
		Check_Low->SetIsChecked(QualityLevel == 0);
	}

	if (Check_Medium)
	{
		Check_Medium->SetIsChecked(QualityLevel == 1);
	}

	if (Check_High)
	{
		Check_High->SetIsChecked(QualityLevel == 2);
	}

	bUpdatingQualityCheck = false;
}

void UGameConfigPopupWidget::RefreshVolumeUI(float Value)
{
	Value = FMath::Clamp(Value, 0.0f, 1.0f);

	if (PB_Volume)
	{
		PB_Volume->SetPercent(Value);
	}
}

void UGameConfigPopupWidget::OnCloseClicked()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}

	RemoveFromParent();

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void UGameConfigPopupWidget::OnLowClicked()
{
	ApplyQuality(0);
}

void UGameConfigPopupWidget::OnMediumClicked()
{
	ApplyQuality(1);
}

void UGameConfigPopupWidget::OnHighClicked()
{
	ApplyQuality(2);
}
