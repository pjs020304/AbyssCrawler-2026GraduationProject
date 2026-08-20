#include "AbyssGameMode.h"
#include "AbyssGameState.h"
#include "AbyssPlayerState.h"
#include "AbyssPlayerController.h"
#include "AbyssDiverCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/Contents/AbyssMissionLight.h"
#include "TitleGameInstance.h"
#include "EngineUtils.h"
#include "Mission/Contents/AbyssMissionArea.h"

AAbyssGameMode::AAbyssGameMode()
{
    // 우리가 만든 클래스들을 기본값으로 지정
    GameStateClass = AAbyssGameState::StaticClass();
    PlayerStateClass = AAbyssPlayerState::StaticClass();
    PlayerControllerClass = AAbyssPlayerController::StaticClass();

    bDelayedStart = false;
}

void AAbyssGameMode::BeginPlay()
{
    Super::BeginPlay();

    StartMainGameFlow();
}

void AAbyssGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer) return;

    // GAS 초기화: PlayerState에 있는 ASC를 갱신
    if (AAbyssPlayerState* PS = NewPlayer->GetPlayerState<AAbyssPlayerState>())
    {
        // NickName
        if (UTitleGameInstance* GI = GetGameInstance<UTitleGameInstance>())
        {
            const FString PlayerKey = NewPlayer->PlayerState
                ? NewPlayer->PlayerState->GetPlayerName()
                : NewPlayer->GetName();

            PS->Nickname = GI->GetSavedNickname(PlayerKey);
        }

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
        FinishGameOver();
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
            SetMissionAreaMarkerVisible(Mission.MissionId, false);

            GS->AddSharedMoney(Mission.RewardMoney);

            UE_LOG(LogTemp, Warning, TEXT("[MissionReward] %s Money +%d"),
                *Mission.MissionId.ToString(),
                Mission.RewardMoney
            );

            UE_LOG(LogTemp, Warning, TEXT("[Mission] Completed: %s"), *Mission.MissionTitle.ToString());

            for (TActorIterator<AAbyssMissionLight> It(GetWorld()); It; ++It)
            {
                AAbyssMissionLight* LightController = *It;

                if (LightController && LightController->MissionId == Mission.MissionId)
                {
                    LightController->SetLightsEnabled(true);
                }
            }

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

void AAbyssGameMode::AddMissionProgressById(FName MissionId, int32 Amount)
{
    if (AAbyssGameState* GS = GetGameState<AAbyssGameState>())
    {
        int32 MissionIndex = INDEX_NONE;

        for (int32 i = 0; i < GS->Missions.Num(); ++i)
        {
            if (GS->Missions[i].MissionId == MissionId)
            {
                MissionIndex = i;
                break;
            }
        }

        if (MissionIndex == INDEX_NONE) return;

        const bool bWasCompleted = GS->Missions[MissionIndex].bCompleted;

        GS->AddMissionProgressById(MissionId, Amount);

        const FAbyssMissionData& Mission = GS->Missions[MissionIndex];

        if (!bWasCompleted && Mission.bCompleted)
        {
            SetMissionAreaMarkerVisible(Mission.MissionId, false);

            GS->AddSharedMoney(Mission.RewardMoney);

            UE_LOG(LogTemp, Warning, TEXT("[MissionReward] %s Money +%d"),
                *Mission.MissionId.ToString(),
                Mission.RewardMoney
            );

            for (TActorIterator<AAbyssMissionLight> It(GetWorld()); It; ++It)
            {
                AAbyssMissionLight* LightController = *It;

                if (LightController && LightController->MissionId == Mission.MissionId)
                {
                    LightController->SetLightsEnabled(true);

                    UE_LOG(LogTemp, Warning, TEXT("[MissionLight] Light ON: %s"),
                        *Mission.MissionId.ToString());
                }
            }

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

void AAbyssGameMode::AcceptMissionById(FName MissionId)
{
    if (AAbyssGameState* GS = GetGameState<AAbyssGameState>())
    {
        GS->AddMissionById(MissionId);

        // 미션을 받으면 해당 MissionId를 가진 Area 마커 켜기
        SetMissionAreaMarkerVisible(MissionId, true);

        UE_LOG(LogTemp, Warning, TEXT("[MissionArea] Try Show Marker: %s"),
            *MissionId.ToString()
        );
    }
}

void AAbyssGameMode::SetMissionAreaMarkerVisible(FName MissionId, bool bVisible)
{
    bool bFoundArea = false;

    for (TActorIterator<AAbyssMissionArea> It(GetWorld()); It; ++It)
    {
        AAbyssMissionArea* Area = *It;
        if (!Area)
        {
            continue;
        }

        UE_LOG(LogTemp, Warning, TEXT("[MissionArea] Check Area MissionId=%s / Target=%s"),
            *Area->GetMissionId().ToString(),
            *MissionId.ToString()
        );

        if (Area->GetMissionId() == MissionId)
        {
            bFoundArea = true;

            Area->SetMissionMarkerVisible(bVisible);

            UE_LOG(LogTemp, Warning, TEXT("[MissionArea] Marker %s: %s"),
                bVisible ? TEXT("ON") : TEXT("OFF"),
                *MissionId.ToString());
        }
    }

    if (!bFoundArea)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionArea] No Area Found For MissionId: %s"),
            *MissionId.ToString()
        );
    }
}

void AAbyssGameMode::StartMainGameFlow()
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS) return;

    GS->SetGamePhase(EAbyssGamePhase::Playing);
    GS->SetRemainingGameTime(GameLimitTime);

    GetWorldTimerManager().SetTimer(
        GameTimerHandle,
        this,
        &AAbyssGameMode::UpdateGameTimer,
        1.0f,
        true
    );

    UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Game Started"));
}

void AAbyssGameMode::CheckGameClearCondition()
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS) return;

    if (GS->GetGamePhase() != EAbyssGamePhase::Playing)
    {
        return;
    }

    if (GS->ProgressPoint >= GS->TargetProgressPoint)
    {
        // 잠수함 귀환
    }
}

void AAbyssGameMode::CheckGameOverCondition()
{
}

void AAbyssGameMode::FinishGameClear()
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS) return;

    if (GS->GetGamePhase() != EAbyssGamePhase::Playing)
    {
        return;
    }

    GS->SetGamePhase(EAbyssGamePhase::Cleared);
    GetWorldTimerManager().ClearTimer(GameTimerHandle);

    UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Game Clear"));

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(PC->GetPawn()))
            {
                //Diver->Client_ShowGameClearUI();
                Diver->Client_PlayFadeOut();
            }
        }
    }

    FTimerHandle EndingTravelTimer;
    GetWorldTimerManager().SetTimer(
        EndingTravelTimer,
        this,
        &AAbyssGameMode::TravelToEndingMap,
        2.0f,
        false
    );
}

void AAbyssGameMode::FinishGameOver()
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS) return;

    if (GS->GetGamePhase() != EAbyssGamePhase::Playing)
    {
        return;
    }

    GS->SetGamePhase(EAbyssGamePhase::GameOver);
    GetWorldTimerManager().ClearTimer(GameTimerHandle);

    UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Game Over"));

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(PC->GetPawn()))
            {
                Diver->Client_ShowGameOverUI();
            }
        }
    }
}

void AAbyssGameMode::OnPlayerEscaped(AController* PlayerController)
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS) return;

    if (GS->GetGamePhase() != EAbyssGamePhase::Playing)
    {
        return;
    }

    if (GS->ProgressPoint < GS->TargetProgressPoint)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameFlow] Cannot escape yet. Progress not enough."));
        return;
    }

    FinishGameClear();
}

void AAbyssGameMode::TravelToEndingMap()
{
    if (!HasAuthority())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FString EndingMapPath = TEXT("/Game/AbyssCrawler/Maps/MainGame/L_Ending_Submarine");

    UE_LOG(LogTemp, Warning, TEXT("[GameFlow] ServerTravel To EndingMap: %s"), *EndingMapPath);

    World->ServerTravel(EndingMapPath);
}

void AAbyssGameMode::UpdateGameTimer()
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS) return;

    if (GS->GetGamePhase() != EAbyssGamePhase::Playing)
    {
        return;
    }

    const float NewTime = GS->GetRemainingGameTime() - 1.0f;
    GS->SetRemainingGameTime(NewTime);

    if (NewTime <= 0.0f)
    {
        FinishGameOver();
    }
}

void AAbyssGameMode::Debug_SetProgressFull()
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS)
    {
        return;
    }

    GS->Debug_SetProgressFull();

    UE_LOG(LogTemp, Warning, TEXT("[Exhibition] Mission Progress set to 100%%"));
}

void AAbyssGameMode::Debug_SetRemainingTime(float NewRemainingTime)
{
    AAbyssGameState* GS = GetGameState<AAbyssGameState>();
    if (!GS)
    {
        return;
    }

    GS->SetRemainingGameTime(NewRemainingTime);

    UE_LOG(LogTemp, Warning, TEXT("[Exhibition] Remaining Time set to %.1f"), NewRemainingTime);
}