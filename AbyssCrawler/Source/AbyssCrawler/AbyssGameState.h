#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h" // GameStateBase 대신 GameState 사용 (MatchState 활용 가능)
#include "AbyssGameState.generated.h"

USTRUCT(BlueprintType)
struct FAbyssMissionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText MissionTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CurrentCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TargetCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardProgressPoint = 10;
};

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
	UPROPERTY(ReplicatedUsing = OnRep_CollectedItems, BlueprintReadOnly, Category = "Abyss Mission")
	int32 CollectedItemsCount;

	// 총 모아야 할 자원 개수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abyss Mission")
	int32 TargetItemsCount = 10;

	UPROPERTY(ReplicatedUsing = OnRep_Missions, EditAnywhere, BlueprintReadOnly, Category = "Abyss Mission")
	TArray<FAbyssMissionData> Missions;

	// 미션 진행도
	UPROPERTY(ReplicatedUsing = OnRep_Missions, BlueprintReadOnly, Category = "Abyss Mission")
	int32 ProgressPoint = 0;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Abyss Mission")
	int32 TargetProgressPoint = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardProgressPoint = 10;

	UFUNCTION()
	void OnRep_CollectedItems();

	UFUNCTION()
	void AddCollectedItem();

	UFUNCTION()
	void OnRep_Missions();

	void AddMissionProgress(int32 MissionIndex, int32 Amount = 1);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};