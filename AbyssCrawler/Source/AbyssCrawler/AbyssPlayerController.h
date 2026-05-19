#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AbyssPlayerController.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAbyssPlayerController();

protected:
	virtual void SetupInputComponent() override;

	void CycleSpectatePlayer();
};
