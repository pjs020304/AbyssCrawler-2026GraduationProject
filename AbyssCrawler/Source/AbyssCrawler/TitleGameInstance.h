// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "TitleGameInstance.generated.h"

class USoundMix;
class USoundClass;
class ULoadingScreenWidget;

UCLASS()
class ABYSSCRAWLER_API UTitleGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;

	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	UPROPERTY()
	TMap<FString, FText> PlayerNicknames;

	void SavePlayerNickname(const FString& PlayerKey, const FText& Nickname);
	FText GetSavedNickname(const FString& PlayerKey) const;

	UFUNCTION(BlueprintCallable)
	void ApplySavedGameConfig();

	UFUNCTION(BlueprintCallable)
	void SaveAndApplyVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Player|Color")
	void SetSelectedPlayerColorIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "Player|Color")
	int32 GetSelectedPlayerColorIndex() const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UUserWidget> WrongPopupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundMix> SettingSoundMix;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loading")
	TSubclassOf<ULoadingScreenWidget> LoadingScreenWidgetClass;

private:
	UPROPERTY()
	int32 SelectedPlayerColorIndex = 0;
};
