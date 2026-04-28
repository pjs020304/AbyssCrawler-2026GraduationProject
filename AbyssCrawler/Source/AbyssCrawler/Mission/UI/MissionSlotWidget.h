// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionSlotWidget.generated.h"

class UTextBlock;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API UMissionSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetMissionData(const FString& Name, int32 Current, int32 Max);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_MissionName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_MissionProgress;
};
