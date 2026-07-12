#include "AbyssPlayerController.h"
#include "AbyssDiverCharacter.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Camera/CameraActor.h"

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

void AAbyssPlayerController::Client_StartEndingSequence_Implementation(
	TSubclassOf<UUserWidget> ClearWidgetClass
)
{
	PendingEndingWidgetClass = ClearWidgetClass;

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	ALevelSequenceActor* FoundSequenceActor = nullptr;

	for (TActorIterator<ALevelSequenceActor> It(GetWorld()); It; ++It)
	{
		FoundSequenceActor = *It;
		break;
	}

	if (!FoundSequenceActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Ending] Client LevelSequenceActor not found"));
		ShowEndingClearUI();
		return;
	}

	ULevelSequencePlayer* SequencePlayer = FoundSequenceActor->GetSequencePlayer();
	if (!SequencePlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Ending] Client SequencePlayer not found"));
		ShowEndingClearUI();
		return;
	}

	SequencePlayer->OnFinished.RemoveDynamic(this, &AAbyssPlayerController::HandleEndingSequenceFinished);
	SequencePlayer->OnFinished.AddDynamic(this, &AAbyssPlayerController::HandleEndingSequenceFinished);

	UE_LOG(LogTemp, Warning, TEXT("[Ending] Client Play Sequence"));

	SequencePlayer->Play();
}

void AAbyssPlayerController::HandleEndingSequenceFinished()
{
	ShowEndingClearUI();
}

void AAbyssPlayerController::ShowEndingClearUI()
{
	if (EndingWidgetRef)
	{
		EndingWidgetRef->RemoveFromParent();
		EndingWidgetRef = nullptr;
	}

	if (PendingEndingWidgetClass)
	{
		EndingWidgetRef = CreateWidget<UUserWidget>(this, PendingEndingWidgetClass);
		if (EndingWidgetRef)
		{
			EndingWidgetRef->AddToViewport(999);
		}
	}

	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;

	if (EndingWidgetRef)
	{
		InputMode.SetWidgetToFocus(EndingWidgetRef->TakeWidget());
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}