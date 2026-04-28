// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/UI/MissionSlotWidget.h"
#include "Components/TextBlock.h"

void UMissionSlotWidget::SetMissionData(const FString& Name, int32 Current, int32 Max)
{
    UE_LOG(LogTemp, Warning, TEXT("[MissionSlot] SetMissionData Called: %s %d/%d"),
        *Name, Current, Max);

    if (!TXT_MissionName)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionSlot] TXT_MissionName is NULL"));
    }

    if (!TXT_MissionProgress)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionSlot] TXT_MissionProgress is NULL"));
    }

    if (TXT_MissionName)
    {
        TXT_MissionName->SetText(FText::FromString(Name));
    }

    if (TXT_MissionProgress)
    {
        const FString Progress = FString::Printf(TEXT("%d / %d"), Current, Max);
        TXT_MissionProgress->SetText(FText::FromString(Progress));
    }
}
