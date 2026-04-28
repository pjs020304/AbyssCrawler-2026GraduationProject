#include "AbyssGameMode.h"
#include "AbyssGameState.h"
#include "AbyssPlayerState.h"
#include "AbyssDiverCharacter.h"
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

void AAbyssGameMode::OnItemCollected()
{
    if (AAbyssGameState* GS = GetGameState<AAbyssGameState>())
    {
        GS->AddCollectedItem();

        // 미션 완료 체크
        if (GS->CollectedItemsCount >= GS->TargetItemsCount)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Mission] COMPLETE!"));

            // 클리어 처리
        }
    }
}

void AAbyssGameMode::OnMissionItemCollected(int32 MissionIndex)
{
    if (AAbyssGameState* GS = GetGameState<AAbyssGameState>())
    {
        GS->AddMissionProgress(MissionIndex, 1);
    }
}

void AAbyssGameMode::AddMissionProgress(int32 MissionIndex, int32 Amount)
{
    if (AAbyssGameState* GS = GetGameState<AAbyssGameState>())
    {
        if (!GS->Missions.IsValidIndex(MissionIndex)) return;

        // 이전 상태
        const bool bWasCompleted = GS->Missions[MissionIndex].bCompleted;

        // 진행도 증가
        GS->AddMissionProgress(MissionIndex, Amount);

        // 이후에 상태 확인
        const FAbyssMissionData& Mission = GS->Missions[MissionIndex];

        // 완료된 경우 실행
        if (!bWasCompleted && Mission.bCompleted)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Mission] Completed: %s"), *Mission.MissionTitle.ToString());

            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                if (APlayerController* PC = It->Get())
                {
                    if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(PC->GetPawn()))
                    {
                        Diver->Client_ShowMissionComplete(Mission.MissionTitle);
                    }
                }
            }
        }
    }
}