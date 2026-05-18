#include "DecoyItem.h"
#include "DecoyActor.h"
#include "AbyssDiverCharacter.h"
#include "TimerManager.h"

void ADecoyItem::UseItem()
{
	Super::UseItem();

	if (!HasAuthority()) return;

	if (OwnerCharacter)
	{
		// Block movement on client
		OwnerCharacter->Client_SetMovementBlocked(true);

		// Start 3 second timer
		GetWorldTimerManager().SetTimer(InstallTimerHandle, this, &ADecoyItem::FinishInstallation, 3.0f, false);
	}
}

void ADecoyItem::FinishInstallation()
{
	if (OwnerCharacter)
	{
		// Unblock movement on client
		OwnerCharacter->Client_SetMovementBlocked(false);

		if (DecoyActorClass)
		{
			FVector SpawnLocation = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorForwardVector() * 100.0f);
			FRotator SpawnRotation = FRotator::ZeroRotator;

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = OwnerCharacter;
			SpawnParams.Instigator = OwnerCharacter;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GetWorld()->SpawnActor<ADecoyActor>(DecoyActorClass, SpawnLocation, SpawnRotation, SpawnParams);
		}

		// Consume item
		OwnerCharacter->ConsumeItem(this);
	}
}
