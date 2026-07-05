// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssDataConsole.generated.h"

class UStaticMeshComponent;
class AAbyssDiverCharacter;

UCLASS()
class ABYSSCRAWLER_API AAbyssDataConsole : public AActor
{
	GENERATED_BODY()
	
public:
	AAbyssDataConsole();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void StartDownload(AAbyssDiverCharacter* Character);

	UFUNCTION(BlueprintCallable)
	void StopDownload(AAbyssDiverCharacter* Character);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FName MissionId = TEXT("ConsoleDataRecovery");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	float DownloadTime = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	float MaxWorkDistance = 250.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Completed, BlueprintReadOnly)
	bool bCompleted = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsWorking = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentDownloadTime = 0.0f;

	UPROPERTY()
	TObjectPtr<AAbyssDiverCharacter> WorkingCharacter;

	UFUNCTION()
	void OnRep_Completed();

	void CompleteDownload();

	bool CanWork(AAbyssDiverCharacter* Character) const;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
