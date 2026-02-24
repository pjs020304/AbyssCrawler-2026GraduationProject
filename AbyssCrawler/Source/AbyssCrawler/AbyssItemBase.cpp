// Fill out your copyright notice in the Description page of Project Settings.


#include "AbyssItemBase.h"

// Sets default values
AAbyssItemBase::AAbyssItemBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AAbyssItemBase::UseItem()
{
	UE_LOG(LogTemp, Log, TEXT("-----Used Item-----: %s"), *ItemName);
}
