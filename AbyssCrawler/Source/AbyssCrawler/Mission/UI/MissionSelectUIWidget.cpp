// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/UI/MissionSelectUIWidget.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "Mission/UI/MissionSelectSlotWidget.h"
#include "AbyssDiverCharacter.h"

void UMissionSelectUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BTN_Confirm)
	{
		BTN_Confirm->OnClicked.AddDynamic(this, &UMissionSelectUIWidget::HandleConfirmClicked);
	}

	if (BTN_Cancel)
	{
		BTN_Cancel->OnClicked.AddDynamic(this, &UMissionSelectUIWidget::HandleCancelClicked);
	}
}

void UMissionSelectUIWidget::InitializeMissionList(const TArray<FAbyssMissionData>& Missions)
{
	UE_LOG(LogTemp, Warning, TEXT("[MissionSelectUI] InitializeMissionList Count=%d"), Missions.Num());

	if (!MissionList)
	{
		UE_LOG(LogTemp, Error, TEXT("[MissionSelectUI] MissionList NULL"));
		return;
	}

	if (!MissionSlotClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[MissionSelectUI] MissionSlotClass NULL"));
		return;
	}

	//if (!MissionList || !MissionSlotClass) return;

	MissionList->ClearChildren();
	SelectedMissionIds.Empty();

	for (const FAbyssMissionData& Mission : Missions)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MissionSelectUI] Add Slot: %s"), *Mission.MissionId.ToString());

		UMissionSelectSlotWidget* NewSlot =
			CreateWidget<UMissionSelectSlotWidget>(this, MissionSlotClass);

		if (!NewSlot) continue;

		NewSlot->SetMissionData(Mission);

		NewSlot->OnMissionSelectClicked.AddDynamic(
			this,
			&UMissionSelectUIWidget::HandleMissionSelected
		);

		MissionList->AddChild(NewSlot);
	}
}

void UMissionSelectUIWidget::HandleMissionSelected(FName MissionId)
{
	// 이미 선택된 경우 해제
	if (SelectedMissionIds.Contains(MissionId))
	{
		SelectedMissionIds.Remove(MissionId);
		UE_LOG(LogTemp, Warning, TEXT("Unselected: %s"), *MissionId.ToString());
		return;
	}

	// 최대 갯수 제한
	if (SelectedMissionIds.Num() >= MaxSelectableCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("Max 3 missions"));
		return;
	}

	SelectedMissionIds.Add(MissionId);

	UE_LOG(LogTemp, Warning, TEXT("Selected: %s"), *MissionId.ToString());

	// 선택된 슬롯 제거
	for (int32 i = 0; i < MissionList->GetChildrenCount(); ++i)
	{
		if (UMissionSelectSlotWidget* SelectSlot =
			Cast<UMissionSelectSlotWidget>(MissionList->GetChildAt(i)))
		{
			if (SelectSlot->GetMissionId() == MissionId)
			{
				MissionList->RemoveChildAt(i);
				break;
			}
		}
	}

	// 클릭 후에도 마우스 입력 유지
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		//InputMode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void UMissionSelectUIWidget::HandleConfirmClicked()
{
	
	if (SelectedMissionIds.Num() == 0) return;

	AAbyssDiverCharacter* Character = Cast<AAbyssDiverCharacter>(GetOwningPlayerPawn());
	if (!Character) return;

	for (const FName& MissionId : SelectedMissionIds)
	{
		Character->Server_AcceptMissionById(MissionId);
	}

	CloseUI();
}

void UMissionSelectUIWidget::HandleCancelClicked()
{
	CloseUI();
}

void UMissionSelectUIWidget::CloseUI()
{
	if (bIsClosing) return;
	bIsClosing = true;

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);

		PC->bShowMouseCursor = false;

		// 막힌 입력 해제
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);

		// 마우스 캡처 복구
		PC->SetMouseLocation(
			GEngine->GameViewport->Viewport->GetSizeXY().X / 2,
			GEngine->GameViewport->Viewport->GetSizeXY().Y / 2
		);
	}

	RemoveFromParent();
}