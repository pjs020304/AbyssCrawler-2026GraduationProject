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

    Missions.Add({ FText::FromString(TEXT("Collect Parts")), 0, 10, false, 30 });
    Missions.Add({ FText::FromString(TEXT("Repair Generator")), 0, 3, false, 40 });
    Missions.Add({ FText::FromString(TEXT("Reach Escape Area")), 0, 1, false, 20 });
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
}

bool AAbyssGameState::ConsumeSharedMoney(int32 Amount)
{
    if (HasAuthority())
    {
        if (SharedMoney >= Amount)
        {
            SharedMoney -= Amount;
            return true;
        }
    }
    return false;
}

void AAbyssGameState::AddSharedMoney(int32 Amount)
{
    if (HasAuthority() && Amount > 0)
    {
        SharedMoney += Amount;
    }
}
