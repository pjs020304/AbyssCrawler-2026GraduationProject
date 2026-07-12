#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AbyssPlayerController.generated.h"

class ALevelSequenceActor;
class UUserWidget;

UCLASS()
class ABYSSCRAWLER_API AAbyssPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAbyssPlayerController();

	UFUNCTION(Client, Reliable)
	void Client_StartEndingSequence(TSubclassOf<UUserWidget> ClearWidgetClass);

	UFUNCTION()
	void HandleEndingSequenceFinished();

	void ShowEndingClearUI();

	UPROPERTY()
	TSubclassOf<UUserWidget> PendingEndingWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> EndingWidgetRef;

protected:
	virtual void SetupInputComponent() override;

	void CycleSpectatePlayer();
};
