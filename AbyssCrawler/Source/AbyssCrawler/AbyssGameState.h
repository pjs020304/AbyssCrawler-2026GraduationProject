#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h" // GameStateBase 대신 GameState 사용 (MatchState 활용 가능)
#include "AbyssGameState.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssGameState : public AGameState
{
	GENERATED_BODY()

public:
	AAbyssGameState();

	// 남은 미션 시간 (초)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss Mission")
	int32 RemainingMissionTime;

	// 현재 수집한 중요 자원 개수
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss Mission")
	int32 CollectedItemsCount;

	// 총 모아야 할 자원 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss Mission")
	int32 TargetItemsCount;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};