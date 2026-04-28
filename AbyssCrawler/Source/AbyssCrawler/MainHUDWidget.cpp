// Fill out your copyright notice in the Description page of Project Settings.


#include "MainHUDWidget.h"
#include "Mission/UI/MissionUIWidget.h"

void UMainHUDWidget::RefreshMissionUI()
{
	if (MissionUI)
	{
		MissionUI->RefreshMissionUI();
	}
}
