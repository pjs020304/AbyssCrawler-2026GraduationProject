#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h" // GameStateBase 대신 GameState 사용 (MatchState 활용 가능)
#include "AbyssGameState.generated.h"

class UAbyssMinimapComponent;

UENUM(BlueprintType)
enum class EAbyssGamePhase : uint8
{
	Waiting     UMETA(DisplayName = "Waiting"),
	Playing     UMETA(DisplayName = "Playing"),
	Cleared     UMETA(DisplayName = "Cleared"),
	GameOver    UMETA(DisplayName = "GameOver")
};

USTRUCT(BlueprintType)
struct FAbyssMissionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MissionId;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardMoney = 100;
};

// 델리게이트 선언 (새로운 돈 액수를 인자로 전달)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, NewMoney);

UCLASS()
class ABYSSCRAWLER_API AAbyssGameState : public AGameState
{
	GENERATED_BODY()

public:
	AAbyssGameState();

	// 미니맵 표시용 위치 스냅샷을 서버에서 모아 복제하는 컴포넌트.
	// GameState가 항상 릴리번트이므로 이 데이터는 거리와 무관하게 전 클라이언트에 도달한다.
	UFUNCTION(BlueprintPure, Category = "Minimap")
	UAbyssMinimapComponent* GetMinimapComponent() const { return MinimapComponent; }

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 RewardMoney = 100;

	UFUNCTION()
	void OnRep_CollectedItems();

	UFUNCTION()
	void AddCollectedItem();

	UFUNCTION()
	void OnRep_Missions();

	void AddMissionProgress(int32 MissionIndex, int32 Amount = 1);

	// 돈이 변경될 때마다 UI를 갱신하기 위한 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Economy")
	FOnMoneyChanged OnMoneyChanged;

	// 돈을 추가하거나 뺄 때 사용할 서버 전용 함수
	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool ConsumeSharedMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddSharedMoney(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetSharedMoney() const { return SharedMoney; }

	UFUNCTION(BlueprintCallable)
	bool AddMission(const FAbyssMissionData& NewMission);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss Mission")
	TArray<FAbyssMissionData> MissionPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abyss Mission")
	int32 MaxActiveMissionCount = 3;

	// 멀티플레이어 변수 동기화 필수 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void AddMissionProgressById(FName MissionId, int32 Amount);

	UFUNCTION(BlueprintCallable)
	bool AddMissionById(FName MissionId);

	// 중복 제외
	UFUNCTION(BlueprintCallable)
	bool HasActiveMission(FName MissionId) const;

	UFUNCTION(BlueprintCallable)
	TArray<FAbyssMissionData> GetAvailableMissions() const;

	UPROPERTY(Replicated)
	TArray<FName> CompletedMissionIds;

	UFUNCTION(BlueprintPure)
	EAbyssGamePhase GetGamePhase() const { return GamePhase; }

	UFUNCTION(BlueprintPure)
	float GetRemainingGameTime() const { return RemainingGameTime; }

	void SetGamePhase(EAbyssGamePhase NewPhase);
	void SetRemainingGameTime(float NewTime);

	void Debug_SetProgressFull();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<UAbyssMinimapComponent> MinimapComponent;

	// 팀이 공유하는 돈 (서버에서 클라이언트로 동기화됨)
	UPROPERTY(ReplicatedUsing = OnRep_SharedMoney, VisibleAnywhere, BlueprintReadOnly, Category = "Economy")
	int32 SharedMoney = 0;

	UFUNCTION()
	void OnRep_SharedMoney();

	UPROPERTY(ReplicatedUsing = OnRep_GamePhase, BlueprintReadOnly, Category = "Game Flow")
	EAbyssGamePhase GamePhase = EAbyssGamePhase::Waiting;

	UPROPERTY(ReplicatedUsing = OnRep_RemainingGameTime, BlueprintReadOnly, Category = "Game Flow")
	float RemainingGameTime = 0.0f;

	UFUNCTION()
	void OnRep_GamePhase();

	UFUNCTION()
	void OnRep_RemainingGameTime();

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EAbyssGamePhase, NewPhase);

	UPROPERTY(BlueprintAssignable)
	FOnGamePhaseChanged OnGamePhaseChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemainingGameTimeChanged, float, NewTime);

	UPROPERTY(BlueprintAssignable)
	FOnRemainingGameTimeChanged OnRemainingGameTimeChanged;
};