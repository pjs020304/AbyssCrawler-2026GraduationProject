#pragma once

#include "CoreMinimal.h"
#include "AbyssItemBase.h"
#include "DecoyItem.generated.h"

class ADecoyActor;

UCLASS()
class ABYSSCRAWLER_API ADecoyItem : public AAbyssItemBase
{
	GENERATED_BODY()
	
public:
	virtual void UseItem() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Decoy")
	TSubclassOf<ADecoyActor> DecoyActorClass;

	FTimerHandle InstallTimerHandle;

	void FinishInstallation();
};
