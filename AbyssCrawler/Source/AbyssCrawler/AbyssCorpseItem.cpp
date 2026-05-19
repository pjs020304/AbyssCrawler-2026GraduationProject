// Fill out your copyright notice in the Description page of Project Settings.


#include "AbyssCorpseItem.h"
#include "Net/UnrealNetwork.h"

AAbyssCorpseItem::AAbyssCorpseItem()
{
	bReplicates = true;
	SetReplicateMovement(true);

	CorpseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CorpseMesh"));
	CorpseMesh->SetupAttachment(RootComponent);
	
	if (ItemMesh)
	{
		ItemMesh->SetVisibility(false);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
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

void AAbyssCorpseItem::InitCorpse(USkeletalMesh* DiverMesh, TArray<UMaterialInterface*> Materials)
{
	if (CorpseMesh)
	{
		CorpseMesh->SetSkeletalMeshAsset(DiverMesh);
		for (int32 i = 0; i < Materials.Num(); ++i)
		{
			CorpseMesh->SetMaterial(i, Materials[i]);
		}

		// Enable Ragdoll Physics
		CorpseMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		CorpseMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CorpseMesh->SetSimulatePhysics(true);
	}
}
