// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionWorkObject.h"
#include "Components/StaticMeshComponent.h"
#include "AbyssDiverCharacter.h"
#include "AbyssGameMode.h"
#include "AbyssSharkCharacter.h"
#include "AbyssGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Mission/UI/MissionInteractPromptWidget.h"

AAbyssMissionWorkObject::AAbyssMissionWorkObject()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	MissionPromptWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("MissionPromptWidgetComp"));
	MissionPromptWidgetComp->SetupAttachment(RootComponent);
	MissionPromptWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	MissionPromptWidgetComp->SetDrawAtDesiredSize(true);
	MissionPromptWidgetComp->SetDrawSize(FVector2D(300.0f, 120.0f));
	MissionPromptWidgetComp->SetVisibility(false);
}

void AAbyssMissionWorkObject::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;

	if (!IsMissionActiveForPrompt())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MissionInteract] Mission not active: %s"),
			*MissionId.ToString());
		return;
	}

	if (bCompleted) return;
	if (bIsWorking) return;

	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
	if (!GS) return;

	if (!GS->HasActiveMission(MissionId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MissionWork] Mission not active: %s"), *MissionId.ToString());
		return;
	}

	WorkingCharacter = Cast<AAbyssDiverCharacter>(InstigatorActor);
	if (!WorkingCharacter) return;

	bIsWorking = true;

	CurrentWorkTime = 0.0f;

	WorkingCharacter->SetCurrentWorkObject(this);

	WorkingCharacter->Client_SetWorkInputBlocked(true);
	WorkingCharacter->Client_ShowWorkUI();
	WorkingCharacter->Client_UpdateWorkProgress(0.0f);

	GetWorldTimerManager().SetTimer(
		WorkTimerHandle,
		this,
		&AAbyssMissionWorkObject::CompleteWork,
		WorkDuration,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("[MissionWork] Work Started"));
}

void AAbyssMissionWorkObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()) return;
	if (!bIsWorking || bCompleted) return;
	if (!WorkingCharacter) return;

	if (IsEnemyNearWorkingCharacter())
	{
		CancelWork();
		return;
	}

	CurrentWorkTime += DeltaTime;

	const float Progress = FMath::Clamp(CurrentWorkTime / WorkDuration, 0.0f, 1.0f);

	WorkingCharacter->Client_UpdateWorkProgress(Progress);
}


void AAbyssMissionWorkObject::CompleteWork()
{
	if (!HasAuthority()) return;

	bIsWorking = false;
	bCompleted = true;

	if (WorkingCharacter)
	{
		WorkingCharacter->Client_UpdateWorkProgress(1.0f);
		WorkingCharacter->Client_HideWorkUI();
		WorkingCharacter->Client_SetWorkInputBlocked(false);

		WorkingCharacter->ClearCurrentWorkObject(this);
	}

	if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
	{
		GM->AddMissionProgressById(MissionId, 1);
	}

	WorkingCharacter = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[MissionWork] Work Completed MissionIndex=%d"), MissionIndex);
}

void AAbyssMissionWorkObject::CancelWork()
{
	if (!HasAuthority()) return;
	if (!bIsWorking) return;

	GetWorldTimerManager().ClearTimer(WorkTimerHandle);

	bIsWorking = false;
	CurrentWorkTime = 0.0f;

	if (WorkingCharacter)
	{
		WorkingCharacter->Client_UpdateWorkProgress(0.0f);
		WorkingCharacter->Client_HideWorkUI();
		WorkingCharacter->Client_SetWorkInputBlocked(false);

		WorkingCharacter->ClearCurrentWorkObject(this);
	}

	WorkingCharacter = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[MissionWork] Work Canceled by Enemy"));
}

bool AAbyssMissionWorkObject::IsEnemyNearWorkingCharacter() const
{
	if (!WorkingCharacter) return false;

	TArray<AActor*> Sharks;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AAbyssSharkCharacter::StaticClass(),
		Sharks
	);

	for (AActor* Shark : Sharks)
	{
		if (!Shark) continue;

		const float Distance = FVector::Dist(
			WorkingCharacter->GetActorLocation(),
			Shark->GetActorLocation()
		);

		if (Distance <= EnemyCancelRadius)
		{
			return true;
		}
	}

	return false;
}

void AAbyssMissionWorkObject::OnFocus_Implementation()
{
	if (MissionPromptWidgetComp)
	{
		MissionPromptWidgetComp->SetVisibility(true);
	}

	UpdateMissionPromptUI();
}

void AAbyssMissionWorkObject::OnLostFocus_Implementation()
{
	if (MissionPromptWidgetComp)
	{
		MissionPromptWidgetComp->SetVisibility(false);
	}
}

void AAbyssMissionWorkObject::UpdateMissionPromptUI()
{
	if (!MissionPromptWidgetComp)
	{
		return;
	}

	UMissionInteractPromptWidget* PromptWidget =
		Cast<UMissionInteractPromptWidget>(MissionPromptWidgetComp->GetUserWidgetObject());

	if (!PromptWidget)
	{
		return;
	}

	const bool bHasMission = IsMissionActiveForPrompt();

	bool bCanInteract = bHasMission;
	FText StateText = bHasMission ? ActiveStateText : InactiveStateText;

	// 만약 오브젝트에 bCompleted 변수가 있다면
	if (bCompleted)
	{
		bCanInteract = false;
		StateText = CompletedStateText;
	}

	PromptWidget->UpdateMissionPrompt(
		PromptObjectName,
		StateText,
		bCanInteract
	);
}

bool AAbyssMissionWorkObject::IsMissionActiveForPrompt() const
{
	if (MissionId.IsNone())
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AAbyssGameState* GS = World->GetGameState<AAbyssGameState>();
	if (!GS)
	{
		return false;
	}

	return GS->HasActiveMission(MissionId);
}


