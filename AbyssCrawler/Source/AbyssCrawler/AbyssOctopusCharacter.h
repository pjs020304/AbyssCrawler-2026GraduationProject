#pragma once

#include "CoreMinimal.h"
#include "AbyssEnemyBase.h"
#include "AbyssOctopusCharacter.generated.h"

class UMaterialInstanceDynamic;
class AAbyssDiverCharacter;

UCLASS()
class ABYSSCRAWLER_API AAbyssOctopusCharacter : public AAbyssEnemyBase
{
	GENERATED_BODY()

public:
	AAbyssOctopusCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// --- Stealth Mechanics ---
	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetStealthMode(bool bIsStealth);

	UPROPERTY(EditDefaultsOnly, Category = "Stealth")
	FName StealthParameterName = FName("Camouflage");

	UPROPERTY()
	UMaterialInstanceDynamic* BodyMaterialInstance;

	// --- Attack Mechanics ---
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void GrabPlayer(AAbyssDiverCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void OnGrabBroken();

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float GrabDamagePerSecond = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	int32 RequiredEscapeClicks = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bIsGrabbing = false;

	UPROPERTY()
	AAbyssDiverCharacter* GrabbedPlayer;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FName AttachBoneName = FName("head");

	// --- Cooldown Mechanics ---
	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool CanGrab() const { return !bIsGrabbing && !bIsGrabCooldown; }

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float GrabCooldownTime = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bIsGrabCooldown = false;

	FTimerHandle GrabCooldownTimerHandle;

	void ResetGrabCooldown();
};
