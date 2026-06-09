// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "TitleGameInstance.generated.h"

/**
 * 
 */
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

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UUserWidget> WrongPopupClass;
};
