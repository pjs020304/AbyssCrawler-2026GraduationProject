#include "AbyssPlayerController.h"
#include "AbyssDiverCharacter.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Net/UnrealNetwork.h"

AAbyssPlayerController::AAbyssPlayerController()
{
}

void AAbyssPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AAbyssPlayerController, SpectateTarget, COND_OwnerOnly);
}

void AAbyssPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 관전 대상 순환: 좌클릭.
	// 캐릭터 입력은 Enhanced Input이지만, 관전 상태에서는 폰이 없어(캐릭터 IMC 미적용)
	// 컨트롤러 레벨의 키 바인딩이 단순하고 확실하다.
	// 살아 있을 때의 좌클릭(아이템 사용)을 먹지 않도록 입력을 소비하지 않는다.
	FInputKeyBinding& Binding = InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AAbyssPlayerController::CycleSpectatePlayer);
	Binding.bConsumeInput = false;
}

void AAbyssPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 부활 등으로 새 폰에 빙의하면 관전 종료
	SpectateTarget = nullptr;
}

bool AAbyssPlayerController::IsSpectating() const
{
	return GetStateName() == NAME_Spectating && GetPawn() == nullptr;
}

// 관전 대상으로 유효한가: 살아 있는 다른 다이버
static bool IsValidSpectateTarget(const APawn* Candidate, const APlayerController* Viewer)
{
	const AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(Candidate);
	return IsValid(Diver) && !Diver->bIsDead && Diver->GetController() != Viewer;
}

void AAbyssPlayerController::StartSpectating()
{
	if (!HasAuthority()) return;

	// 엔진 표준 관전 상태 (폰 빙의 해제 + 관전 폰). 서버/클라 양쪽 상태를 맞춘다.
	ChangeState(NAME_Spectating);
	ClientGotoState(NAME_Spectating);

	SelectNextSpectateTarget();
}

void AAbyssPlayerController::CycleSpectatePlayer()
{
	if (!IsSpectating()) return;

	Server_CycleSpectateTarget();
}

void AAbyssPlayerController::Server_CycleSpectateTarget_Implementation()
{
	if (!IsSpectating()) return;

	SelectNextSpectateTarget();
}

void AAbyssPlayerController::SelectNextSpectateTarget()
{
	if (!HasAuthority() || !GetWorld()) return;

	TArray<APawn*> AlivePlayers;
	for (TActorIterator<AAbyssDiverCharacter> It(GetWorld()); It; ++It)
	{
		if (IsValidSpectateTarget(*It, this))
		{
			AlivePlayers.Add(*It);
		}
	}

	if (AlivePlayers.Num() == 0)
	{
		SetSpectateTarget(nullptr);
		return;
	}

	const int32 CurrentIndex = AlivePlayers.IndexOfByKey(SpectateTarget.Get());
	const int32 NextIndex = (CurrentIndex + 1) % AlivePlayers.Num(); // 못 찾으면(-1) 0번부터
	SetSpectateTarget(AlivePlayers[NextIndex]);
}

void AAbyssPlayerController::SetSpectateTarget(APawn* NewTarget)
{
	if (SpectateTarget == NewTarget) return;

	SpectateTarget = NewTarget;

	// 서버에도 뷰 타깃을 맞춰 둬야 관전 대상 주변 액터의 네트워크 릴리번시가 유지된다.
	// 리슨 호스트 자신은 OnRep이 오지 않으므로 여기서 바로 카메라를 적용한다.
	ApplySpectateViewTarget(true);
}

void AAbyssPlayerController::OnRep_SpectateTarget()
{
	ApplySpectateViewTarget(true);
}

void AAbyssPlayerController::ApplySpectateViewTarget(bool bBlend)
{
	if (!IsValid(SpectateTarget) || !IsSpectating()) return;

	if (bBlend)
	{
		SetViewTargetWithBlend(SpectateTarget, 0.5f);
	}
	else
	{
		SetViewTarget(SpectateTarget);
	}
}

void AAbyssPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// [서버] 관전 대상이 죽거나 사라지면 다음 생존자로 자동 전환
	if (HasAuthority() && IsSpectating() && !IsValidSpectateTarget(SpectateTarget, this))
	{
		SelectNextSpectateTarget();
	}
}

void AAbyssPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// [로컬] 관전 상태 진입(ClientGotoState) 시 엔진이 뷰 타깃을 관전 폰으로 되돌릴 수 있으므로,
	// 관전 대상과 카메라가 어긋나면 다시 맞춘다. (블렌드 중인 대상은 건드리지 않음)
	if (IsSpectating() && IsValid(SpectateTarget) && PlayerCameraManager)
	{
		const bool bAlreadyTarget = GetViewTarget() == SpectateTarget
			|| PlayerCameraManager->PendingViewTarget.Target == SpectateTarget;
		if (!bAlreadyTarget)
		{
			ApplySpectateViewTarget(false);
		}
	}
}

void AAbyssPlayerController::Client_ShowGameOverUI_Implementation(TSubclassOf<UUserWidget> GameOverWidgetClass)
{
	if (GameOverWidgetRef)
	{
		GameOverWidgetRef->RemoveFromParent();
		GameOverWidgetRef = nullptr;
	}

	if (GameOverWidgetClass)
	{
		GameOverWidgetRef = CreateWidget<UUserWidget>(this, GameOverWidgetClass);
		if (GameOverWidgetRef)
		{
			GameOverWidgetRef->AddToViewport(999);
		}
	}

	SetPause(false);
	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->DisableInput(this);
	}
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