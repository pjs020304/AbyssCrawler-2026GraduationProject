// Fill out your copyright notice in the Description page of Project Settings.


#include "MainHUDWidget.h"
#include "Mission/UI/MissionUIWidget.h"
#include "AbyssGameState.h"
#include "Chat/UI/Chatting.h"
#include "Blueprint/WidgetTree.h"

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

        GS->OnRemainingGameTimeChanged.AddDynamic(this, &UMainHUDWidget::UpdateRemainingTimeDisplay);
        UpdateRemainingTimeDisplay(GS->GetRemainingGameTime());
    }
}

void UMainHUDWidget::UpdateMoneyDisplay(int32 NewMoney)
{
    // 블루프린트 이벤트를 호출하여 텍스트를 갱신합니다.
    OnUpdateMoneyText(NewMoney);
}

void UMainHUDWidget::UpdateRemainingTimeDisplay(float NewTime)
{
    const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(NewTime));
    const int32 Minutes = TotalSeconds / 60;
    const int32 Seconds = TotalSeconds % 60;

    const FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);

    OnUpdateRemainingTimeText(FText::FromString(TimeString));
}

UChatting* UMainHUDWidget::GetChattingUI() const
{
    if (!WidgetTree) return nullptr;

    return Cast<UChatting>(WidgetTree->FindWidget(TEXT("ChattingUI")));
}

void UMainHUDWidget::AddChatMessage(const FString& Message)
{
    if (UChatting* Chat = GetChattingUI())
    {
        Chat->AddChatMessage(Message);
    }
}

TSharedPtr<SWidget> UMainHUDWidget::GetChatInputTextObject()
{
    if (UChatting* Chat = GetChattingUI())
    {
        return Chat->GetChatInputTextObject();
    }

    return nullptr;
}