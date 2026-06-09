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
	ADecoyItem();

	virtual void UseItem() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Decoy")
	TSubclassOf<ADecoyActor> DecoyActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decoy|Animation")
	class UAnimMontage* InstallAnimMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decoy|Animation")
	FName InstallMontageSection = NAME_None;

	FTimerHandle InstallTimerHandle;

	bool bIsInstalling = false;
	float InstallTimeElapsed = 0.0f;

	void FinishInstallation();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayInstallAnim();
};
