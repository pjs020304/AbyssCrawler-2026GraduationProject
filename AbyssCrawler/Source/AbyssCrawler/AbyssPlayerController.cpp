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

	// 관전 대상 순환: 좌클릭.
	// 캐릭터 입력은 Enhanced Input이지만, 관전 상태에서는 폰이 없어(캐릭터 IMC 미적용)
	// 컨트롤러 레벨의 키 바인딩이 단순하고 확실하다.
	// (기존의 "UseItemAction" BindAction은 레거시 액션 매핑에 존재하지 않아 제거)
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
