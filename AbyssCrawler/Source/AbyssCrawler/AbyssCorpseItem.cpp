// Fill out your copyright notice in the Description page of Project Settings.


#include "AbyssCorpseItem.h"
#include "Net/UnrealNetwork.h"

AAbyssCorpseItem::AAbyssCorpseItem()
{
	bReplicates = true;
	SetReplicateMovement(true);
}

void AAbyssCorpseItem::SetDeadPlayerState(APlayerState* InPlayerState)
{
	DeadPlayerState = InPlayerState;
}

void AAbyssCorpseItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssCorpseItem, DeadPlayerState);
}
