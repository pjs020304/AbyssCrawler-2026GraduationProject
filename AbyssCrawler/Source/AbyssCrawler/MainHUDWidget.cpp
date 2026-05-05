// Fill out your copyright notice in the Description page of Project Settings.


#include "MainHUDWidget.h"
#include "Mission/UI/MissionUIWidget.h"
#include "AbyssGameState.h"

void UMainHUDWidget::RefreshMissionUI()
{
	if (MissionUI)
	{
		MissionUI->RefreshMissionUI();
	}
}

void UMainHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // GameState를 가져와서 이벤트에 바인딩합니다.
    if (AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>())
    {
        GS->OnMoneyChanged.AddDynamic(this, &UMainHUDWidget::UpdateMoneyDisplay);

        // 초기값 표시
        UpdateMoneyDisplay(GS->GetSharedMoney());
    }
}

void UMainHUDWidget::UpdateMoneyDisplay(int32 NewMoney)
{
    // 블루프린트 이벤트를 호출하여 텍스트를 갱신합니다.
    OnUpdateMoneyText(NewMoney);
}