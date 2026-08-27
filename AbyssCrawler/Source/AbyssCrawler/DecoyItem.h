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
	// 설치가 끝나 미끼가 실제로 소환되는 순간의 효과음.
	// 설치를 "시작"할 때의 소리는 베이스의 UseSound가 담당한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> InstallCompleteSound;

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
