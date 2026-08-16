#include "AbyssGameState.h"
#include "AbyssDiverCharacter.h"
#include "MainHUDWidget.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h" // [중요] 리플리케이션을 위해 필수!

AAbyssGameState::AAbyssGameState()
{
    // 변수 초기화
    RemainingMissionTime = 600; // 예: 10분
    CollectedItemsCount = 0;
    TargetItemsCount = 10;

    //Missions.Add({ FText::FromString(TEXT("Collect Parts")), 0, 10, false, 30 });
    //Missions.Add({ FText::FromString(TEXT("Repair Generator")), 0, 3, false, 40 });
    //Missions.Add({ FText::FromString(TEXT("Reach Escape Area")), 0, 1, false, 20 });
}

void AAbyssGameState::OnRep_CollectedItems()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameState] OnRep Called"));

    // UI 갱신 트리거
    
    if (GetWorld())
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (!PC) return;

        APawn* Pawn = PC->GetPawn();
        if (!Pawn) return;

        AAbyssDiverCharacter* Character = Cast<AAbyssDiverCharacter>(Pawn);
        if (!Character) return;

        if (Character->MainHUDRef)
        {
            Character->MainHUDRef->RefreshMissionUI();
        }
    }
    
}

void AAbyssGameState::AddCollectedItem()
{
    CollectedItemsCount++;

    UE_LOG(LogTemp, Warning, TEXT("[GameState] Collected: %d"), CollectedItemsCount);

    // 서버는 OnRep를 안 받으니까 필요하면 여기서 처리
    OnRep_CollectedItems();
}

void AAbyssGameState::OnRep_Missions()
{
    UE_LOG(LogTemp, Warning, TEXT("[Mission] OnRep_Missions"));

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (AAbyssDiverCharacter* Character = Cast<AAbyssDiverCharacter>(PC->GetPawn()))
        {
            if (Character->MainHUDRef)
            {
                Character->MainHUDRef->RefreshMissionUI();
            }
        }
    }
}

void AAbyssGameState::AddMissionProgress(int32 MissionIndex, int32 Amount)
{
    if (!HasAuthority()) return;
    if (!Missions.IsValidIndex(MissionIndex)) return;
    if (Missions[MissionIndex].bCompleted) return;

    Missions[MissionIndex].CurrentCount += Amount;

    if (Missions[MissionIndex].CurrentCount >= Missions[MissionIndex].TargetCount)
    {
        Missions[MissionIndex].CurrentCount = Missions[MissionIndex].TargetCount;
        Missions[MissionIndex].bCompleted = true;

        ProgressPoint += Missions[MissionIndex].RewardProgressPoint;
        ProgressPoint = FMath::Clamp(ProgressPoint, 0, TargetProgressPoint);

        UE_LOG(LogTemp, Warning, TEXT("[Mission] Complete: %s / ProgressPoint=%d"),
            *Missions[MissionIndex].MissionTitle.ToString(),
            ProgressPoint);
    }

    OnRep_Missions();
}

bool AAbyssGameState::AddMission(const FAbyssMissionData& NewMission)
{
    if (!HasAuthority()) return false;

    if (Missions.Num() >= MaxActiveMissionCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Mission] Cannot add mission. Mission list full."));
        return false;
    }

    Missions.Add(NewMission);

    UE_LOG(LogTemp, Warning, TEXT("[Mission] Added: %s"), *NewMission.MissionTitle.ToString());

    OnRep_Missions();
    return true;
}

// Replicated 변수가 있다면 이 함수를 반드시 구현해야 함
void AAbyssGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 헤더에서 Replicated로 선언한 변수들을 등록
    DOREPLIFETIME(AAbyssGameState, RemainingMissionTime);
    DOREPLIFETIME(AAbyssGameState, CollectedItemsCount);
    DOREPLIFETIME(AAbyssGameState, Missions);
    DOREPLIFETIME(AAbyssGameState, ProgressPoint);
    DOREPLIFETIME(AAbyssGameState, TargetProgressPoint);
    DOREPLIFETIME(AAbyssGameState, CompletedMissionIds);
    DOREPLIFETIME(AAbyssGameState, SharedMoney);
    DOREPLIFETIME(AAbyssGameState, GamePhase);
    DOREPLIFETIME(AAbyssGameState, RemainingGameTime);
}

bool AAbyssGameState::ConsumeSharedMoney(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0)
    {
        return false;
    }

    if (SharedMoney < Amount)
    {
        return false;
    }

    SharedMoney -= Amount;

    UE_LOG(LogTemp, Warning, TEXT("[Money] Consume %d / Total=%d"), Amount, SharedMoney);

    OnRep_SharedMoney();

    return true;
}


void AAbyssGameState::AddSharedMoney(int32 Amount)
{
    if (HasAuthority() && Amount > 0)
    {
        SharedMoney += Amount;

        UE_LOG(LogTemp, Warning, TEXT("[Money] Add %d / Total=%d"), Amount, SharedMoney);

        // 서버는 OnRep를 받지 않으므로 직접 호출
        OnRep_SharedMoney();
    }
}

void AAbyssGameState::SetGamePhase(EAbyssGamePhase NewPhase)
{
    if (!HasAuthority()) return;

    GamePhase = NewPhase;
    OnRep_GamePhase();
}

void AAbyssGameState::SetRemainingGameTime(float NewTime)
{
    if (!HasAuthority()) return;

    RemainingGameTime = NewTime;
    OnRep_RemainingGameTime();
}

void AAbyssGameState::OnRep_SharedMoney()
{
    // 서버로부터 새로운 돈 데이터가 도착하면 UI에 방송합니다.
    OnMoneyChanged.Broadcast(SharedMoney);
}

void AAbyssGameState::OnRep_GamePhase()
{
    OnGamePhaseChanged.Broadcast(GamePhase);
}

void AAbyssGameState::OnRep_RemainingGameTime()
{
    OnRemainingGameTimeChanged.Broadcast(RemainingGameTime);
}


void AAbyssGameState::AddMissionProgressById(FName MissionId, int32 Amount)
{
    if (!HasAuthority()) return;

    for (FAbyssMissionData& Mission : Missions)
    {
        if (Mission.MissionId == MissionId && !Mission.bCompleted)
        {
            Mission.CurrentCount += Amount;

            if (Mission.CurrentCount >= Mission.TargetCount)
            {
                Mission.CurrentCount = Mission.TargetCount;
                Mission.bCompleted = true;

                if (!CompletedMissionIds.Contains(Mission.MissionId))
                {
                    CompletedMissionIds.Add(Mission.MissionId);
                }

                ProgressPoint += Mission.RewardProgressPoint;
                ProgressPoint = FMath::Clamp(ProgressPoint, 0, TargetProgressPoint);

                UE_LOG(LogTemp, Warning, TEXT("[Mission] Completed: %s"), *Mission.MissionId.ToString());
            }

            break;
        }
    }

    OnRep_Missions();
}

bool AAbyssGameState::AddMissionById(FName MissionId)
{
    if (!HasAuthority())
    {
        return false;
    }

    int32 ActiveMissionCount = 0;

    for (const FAbyssMissionData& Mission : Missions)
    {
        if (!Mission.bCompleted)
        {
            ActiveMissionCount++;
        }
    }

    if (ActiveMissionCount >= MaxActiveMissionCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Mission] Active mission list full"));
        return false;
    }

    // 이미 활성화된 미션 중복 방지
    for (const FAbyssMissionData& Mission : Missions)
    {
        if (Mission.MissionId == MissionId && !Mission.bCompleted)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Mission] Already active: %s"), *MissionId.ToString());
            return false;
        }
    }

    // 이미 완료한 미션 다시 받지 않게 유지
    if (CompletedMissionIds.Contains(MissionId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Mission] Already completed: %s"), *MissionId.ToString());
        return false;
    }

    for (const FAbyssMissionData& PoolMission : MissionPool)
    {
        if (PoolMission.MissionId == MissionId)
        {
            FAbyssMissionData NewMission = PoolMission;
            NewMission.CurrentCount = 0;
            NewMission.bCompleted = false;

            Missions.Add(NewMission);

            UE_LOG(LogTemp, Warning, TEXT("[Mission] Added: %s"), *MissionId.ToString());

            OnRep_Missions();
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[Mission] MissionId not found: %s"), *MissionId.ToString());
    return false;
}

bool AAbyssGameState::HasActiveMission(FName MissionId) const
{
    for (const FAbyssMissionData& Mission : Missions)
    {
        if (Mission.MissionId == MissionId && !Mission.bCompleted)
        {
            return true;
        }
    }

    return false;
}

TArray<FAbyssMissionData> AAbyssGameState::GetAvailableMissions() const
{
    TArray<FAbyssMissionData> Result;

    for (const FAbyssMissionData& PoolMission : MissionPool)
    {
        if (CompletedMissionIds.Contains(PoolMission.MissionId))
        {
            continue;
        }

        bool bAlreadyActive = false;

        for (const FAbyssMissionData& ActiveMission : Missions)
        {
            if (ActiveMission.MissionId == PoolMission.MissionId && !ActiveMission.bCompleted)
            {
                bAlreadyActive = true;
                break;
            }
        }

        if (!bAlreadyActive)
        {
            Result.Add(PoolMission);
        }
    }

    return Result;
}

void AAbyssGameState::Debug_SetProgressFull()
{
    if (!HasAuthority())
    {
        return;
    }

    ProgressPoint += 10;

    // 서버에서는 OnRep이 자동 호출되지 않으므로 직접 UI 갱신용 함수 호출
    OnRep_Missions();

    UE_LOG(LogTemp, Warning, TEXT("[Exhibition] ProgressPoint = %d / %d"),
        ProgressPoint,
        TargetProgressPoint);
}