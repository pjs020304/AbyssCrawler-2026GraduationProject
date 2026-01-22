#include "AbyssGameMode.h"
#include "AbyssGameState.h"
#include "AbyssPlayerState.h"
#include "UObject/ConstructorHelpers.h"

AAbyssGameMode::AAbyssGameMode()
{
    // 우리가 만든 클래스들을 기본값으로 지정
    GameStateClass = AAbyssGameState::StaticClass();
    PlayerStateClass = AAbyssPlayerState::StaticClass();

    
}

void AAbyssGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // GAS 초기화: PlayerState에 있는 ASC를 갱신
    if (AAbyssPlayerState* PS = NewPlayer->GetPlayerState<AAbyssPlayerState>())
    {
        // AbilitySystemComponent 초기화 로직 (InitAbilityActorInfo 등)
        // 보통 캐릭터의 PossessedBy에서 호출하지만, 여기서 확실히 처리할 수도 있음
    }
}

void AAbyssGameMode::OnPlayerDied(AController* DeadPlayer)
{
    // 1. 해당 플레이어의 PlayerState 가져오기
    if (AAbyssPlayerState* PS = DeadPlayer->GetPlayerState<AAbyssPlayerState>())
    {
        PS->bIsAlive = false;
    }

    // 2. 남은 생존자가 있는지 확인 (전멸 시 게임 오버)
    bool bAnyAlive = false;
    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (AAbyssPlayerState* AbyssPS = Cast<AAbyssPlayerState>(PS))
        {
            if (AbyssPS->bIsAlive)
            {
                bAnyAlive = true;
                break;
            }
        }
    }

    if (!bAnyAlive)
    {
        // GameOver Logic
    }
}