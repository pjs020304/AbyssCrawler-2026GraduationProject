// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameConfigPopupWidget.generated.h"

class USlider;
class UCheckBox;
class UButton;
class USoundMix;
class USoundClass;
class UProgressBar;
class UTextBlock;

UCLASS()
class ABYSSCRAWLER_API UGameConfigPopupWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Volume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Low;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Medium;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_High;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> Check_Low;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> Check_Medium;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> Check_High;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Close;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Volume;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundMix> SettingSoundMix;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundClass> MasterSoundClass;

private:
	UFUNCTION()
	void OnVolumeChanged(float Value);

	UFUNCTION()
	void OnLowChecked(bool bIsChecked);

	UFUNCTION()
	void OnMediumChecked(bool bIsChecked);

	UFUNCTION()
	void OnHighChecked(bool bIsChecked);

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnLowClicked();

	UFUNCTION()
	void OnMediumClicked();

	UFUNCTION()
	void OnHighClicked();

private:
	void ApplyVolume(float Value);
	void ApplyQuality(int32 QualityLevel);
	void RefreshQualityChecks(int32 QualityLevel);
	void RefreshVolumeUI(float Value);

private:
	bool bUpdatingQualityCheck = false;
};
