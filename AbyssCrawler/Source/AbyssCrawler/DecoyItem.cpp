#include "DecoyItem.h"
#include "DecoyActor.h"
#include "AbyssDiverCharacter.h"
#include "TimerManager.h"

ADecoyItem::ADecoyItem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ADecoyItem::UseItem()
{
	Super::UseItem();

	if (!HasAuthority()) return;

	if (OwnerCharacter)
	{
		// Block movement on client
		OwnerCharacter->Client_SetMovementBlocked(true);

		// Play animation on all clients
		Multicast_PlayInstallAnim();

		// Show UI
		OwnerCharacter->Client_ShowWorkUI();

		// Set state
		bIsInstalling = true;
		InstallTimeElapsed = 0.0f;
		OwnerCharacter->SetWorkType(EAbyssWorkType::ItemInstall);

		// Start 3 second timer
		GetWorldTimerManager().SetTimer(InstallTimerHandle, this, &ADecoyItem::FinishInstallation, 3.0f, false);
	}
}

void ADecoyItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bIsInstalling && OwnerCharacter)
	{
		InstallTimeElapsed += DeltaTime;
		float Progress = FMath::Clamp(InstallTimeElapsed / 3.0f, 0.0f, 1.0f);
		OwnerCharacter->Client_UpdateWorkProgress(Progress);
	}
}

void ADecoyItem::Multicast_PlayInstallAnim_Implementation()
{
	if (OwnerCharacter && InstallAnimMontage)
	{
		OwnerCharacter->PlayAnimMontage(InstallAnimMontage, 1.0f, InstallMontageSection);
	}
}

void ADecoyItem::FinishInstallation()
{
	if (OwnerCharacter)
	{
		// Reset state
		bIsInstalling = false;
		OwnerCharacter->SetWorkType(EAbyssWorkType::None);

		// Hide UI
		OwnerCharacter->Client_HideWorkUI();

		// Unblock movement on client
		OwnerCharacter->Client_SetMovementBlocked(false);

		PlayItemSound(InstallCompleteSound);

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
