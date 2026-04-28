// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/UI/MissionUIWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "MissionSlotWidget.h"
#include "AbyssGameState.h"

void UMissionUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshMissionUI();
}

void UMissionUIWidget::RefreshMissionUI()
{
    if (!MissionList) return;

    MissionList->ClearChildren();

    AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
    if (!GS || !MissionSlotClass) return;

    // 진행도 표시
    if (TXT_ProgressPercent)
    {
        const float ProgressRatio = GS->TargetProgressPoint > 0
            ? static_cast<float>(GS->ProgressPoint) / static_cast<float>(GS->TargetProgressPoint)
            : 0.0f;

        const int32 ProgressPercent = FMath::RoundToInt(ProgressRatio * 100.0f);

        TXT_ProgressPercent->SetText(
            FText::FromString(FString::Printf(TEXT("Progress %d%%"), ProgressPercent))
        );
    }

    int32 DisplayCount = 0;

    for (int32 i = 0; i < GS->Missions.Num(); ++i)
    {
        const FAbyssMissionData& Mission = GS->Missions[i];

        // 완료된 미션 패스
        if (Mission.bCompleted)
        {
            continue;
        }

        UMissionSlotWidget* MissionSlot = CreateWidget<UMissionSlotWidget>(this, MissionSlotClass);
        if (!MissionSlot) continue;

        MissionSlot->SetMissionData(
            Mission.MissionTitle.ToString(),
            Mission.CurrentCount,
            Mission.TargetCount
        );

        MissionList->AddChild(MissionSlot);

        DisplayCount++;

        // 최대 3개 까지
        if (DisplayCount >= 3)
        {
            break;
        }
    }
}