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

	// 플레이어 접속 시 호출 (GAS 초기화 타이밍)
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 플레이어 사망 처리 (관전 모드 전환 등)
	virtual void OnPlayerDied(AController* DeadPlayer);

	// 게임 클리어 조건 체크
	//void CheckMissionComplete();

protected:
	// 시작 시 미션 타이머 가동 등
	//virtual void BeginPlay() override;
};