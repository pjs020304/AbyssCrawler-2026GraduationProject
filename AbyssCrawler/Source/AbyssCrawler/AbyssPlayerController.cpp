#include "AbyssPlayerController.h"
#include "AbyssDiverCharacter.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AAbyssPlayerController::AAbyssPlayerController()
{
}

void AAbyssPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Bind Left Mouse Click to cycle spectator targets
	InputComponent->BindAction("UseItemAction", IE_Pressed, this, &AAbyssPlayerController::CycleSpectatePlayer);
	// We bind to the action name used for Left Click in the project (usually PrimaryAction or UseItemAction).
	// In AbyssDiverCharacter, the left click is mapped to "UseItemAction" or similar in Enhanced Input, 
	// but Enhanced Input is bound in Character. APlayerController uses legacy or enhanced. 
	// To be safe and simple for spectator, we can also bind standard LeftMouseButton key:
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AAbyssPlayerController::CycleSpectatePlayer);
}

void AAbyssPlayerController::CycleSpectatePlayer()
{
	// Only allow cycling if we are spectating (or dead)
	if (GetStateName() != NAME_Spectating)
	{
		return;
	}

	if (!GetWorld()) return;

	TArray<AAbyssDiverCharacter*> AlivePlayers;
	
	for (TActorIterator<AAbyssDiverCharacter> It(GetWorld()); It; ++It)
	{
		AAbyssDiverCharacter* Diver = *It;
		if (Diver && !Diver->bIsDead)
		{
			AlivePlayers.Add(Diver);
		}
	}

	if (AlivePlayers.Num() == 0)
	{
		return; // No one left to spectate
	}

	// Find current view target in the array
	AActor* CurrentTarget = GetViewTarget();
	int32 CurrentIndex = -1;

	for (int32 i = 0; i < AlivePlayers.Num(); ++i)
	{
		if (AlivePlayers[i] == CurrentTarget)
		{
			CurrentIndex = i;
			break;
		}
	}

	// Get next index
	int32 NextIndex = (CurrentIndex + 1) % AlivePlayers.Num();
	
	// Switch camera smoothly
	SetViewTargetWithBlend(AlivePlayers[NextIndex], 0.5f);
}
