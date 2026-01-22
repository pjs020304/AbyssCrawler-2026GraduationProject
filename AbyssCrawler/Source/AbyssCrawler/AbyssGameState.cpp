#include "AbyssGameState.h"
#include "Net/UnrealNetwork.h" // [중요] 리플리케이션을 위해 필수!

AAbyssGameState::AAbyssGameState()
{
    // 변수 초기화
    RemainingMissionTime = 600; // 예: 10분
    CollectedItemsCount = 0;
    TargetItemsCount = 10;
}

// [핵심] Replicated 변수가 있다면 이 함수를 반드시 구현해야 함
void AAbyssGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 헤더에서 Replicated로 선언한 변수들을 등록
    DOREPLIFETIME(AAbyssGameState, RemainingMissionTime);
    DOREPLIFETIME(AAbyssGameState, CollectedItemsCount);
}