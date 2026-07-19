// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssDataConsole.generated.h"

class UStaticMeshComponent;
class AAbyssDiverCharacter;
class UWidgetComponent;
class UMissionInteractPromptWidget;

UCLASS()
class ABYSSCRAWLER_API AAbyssDataConsole : public AActor, public IAbyssInteractionInterface
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

	// InterectPrompt
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> MissionPromptWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText PromptObjectName = FText::FromString(TEXT("Mission Object"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText ActiveStateText = FText::FromString(TEXT("Hold E"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText InactiveStateText = FText::FromString(TEXT("Mission Not Accepted"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Prompt")
	FText CompletedStateText = FText::FromString(TEXT("Completed"));

	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

	void UpdateMissionPromptUI();
	bool IsMissionActiveForPrompt() const;


public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
