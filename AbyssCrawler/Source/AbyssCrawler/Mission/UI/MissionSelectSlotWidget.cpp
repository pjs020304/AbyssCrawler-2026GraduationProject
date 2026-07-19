#include "Mission/UI/MissionSelectSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"




void UMissionSelectSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BTN_Select)
	{
		BTN_Select->OnClicked.AddDynamic(this, &UMissionSelectSlotWidget::HandleSelectClicked);
	}
}

void UMissionSelectSlotWidget::SetMissionData(const FAbyssMissionData& InMissionData)
{
	CachedMissionId = InMissionData.MissionId;

	if (TXT_MissionName)
	{
		TXT_MissionName->SetText(InMissionData.MissionTitle);
	}

	if (TXT_MissionInfo)
	{
		TXT_MissionInfo->SetText(
			FText::FromString(
				FString::Printf(TEXT("Goal: %d"), InMissionData.TargetCount)
			)
		);
	}

	if (TXT_Reward)
	{
		TXT_Reward->SetText(
			FText::FromString(
				FString::Printf(TEXT("Reward: %d%%"), InMissionData.RewardProgressPoint)
			)
		);
	}

	if (TXT_Money)
	{
		TXT_Money->SetText(
			FText::FromString(
				FString::Printf(TEXT("Money: %d"), InMissionData.RewardMoney)
			)
		);
	}
}

void UMissionSelectSlotWidget::HandleSelectClicked()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}

	OnMissionSelectClicked.Broadcast(CachedMissionId);
}