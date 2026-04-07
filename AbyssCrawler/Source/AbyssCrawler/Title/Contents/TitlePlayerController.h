// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"

class UTitleWidget;

/**
 * 
 */
UCLASS()
class ABYSSCRAWLER_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UTitleWidget> TitleWidgetClass;

	UPROPERTY()
	TObjectPtr<UTitleWidget> TitleWidget;
	
};
