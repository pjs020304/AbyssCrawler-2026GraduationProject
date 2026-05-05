// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssMissionArea.generated.h"

class UBoxComponent;

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionArea : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionArea();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 MissionIndex = 2;

	UPROPERTY(Replicated)
	bool bTriggered = false;

	UPROPERTY(EditAnywhere)
	FName MissionId;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
