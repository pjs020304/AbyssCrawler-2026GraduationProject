#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "AbyssGameMode.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AAbyssGameMode();

	virtual void BeginPlay() override;

	// 플레이어 접속 시 호출 (GAS 초기화 타이밍)
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 플레이어 사망 처리 (관전 모드 전환 등)
	virtual void OnPlayerDied(AController* DeadPlayer);

	UFUNCTION()
	void OnItemCollected();

	// 게임 클리어 조건 체크
	//void CheckMissionComplete();

	void OnMissionItemCollected(int32 MissionIndex);

	void AddMissionProgress(int32 MissionIndex, int32 Amount = 1);

	UFUNCTION(BlueprintCallable)
	void AddMissionProgressById(FName MissionId, int32 Amount);

	void AcceptMissionById(FName MissionId);

	void SetMissionAreaMarkerVisible(FName MissionId, bool bVisible);

	UFUNCTION(BlueprintCallable)
	void StartMainGameFlow();

	UFUNCTION(BlueprintCallable)
	void CheckGameClearCondition();

	UFUNCTION(BlueprintCallable)
	void CheckGameOverCondition();

	UFUNCTION(BlueprintCallable)
	void FinishGameClear();

	UFUNCTION(BlueprintCallable)
	void FinishGameOver();

	UFUNCTION(BlueprintCallable)
	void OnPlayerEscaped(AController* PlayerController);

	void TravelToEndingMap();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	float GameLimitTime = 1200.0f; // 20분

	FTimerHandle GameTimerHandle;

	void UpdateGameTimer();
};