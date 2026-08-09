// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionUIWidget.generated.h"

class UVerticalBox;
class UMissionSlotWidget;
class UTextBlock;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API UMissionUIWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	// 미션 리스트 갱신
	UFUNCTION(BlueprintCallable)
	void RefreshMissionUI();

protected:
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* MissionList;

	UPROPERTY(EditAnywhere, Category = "Mission")
	TSubclassOf<UMissionSlotWidget> MissionSlotClass;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_ProgressPercent;

};
