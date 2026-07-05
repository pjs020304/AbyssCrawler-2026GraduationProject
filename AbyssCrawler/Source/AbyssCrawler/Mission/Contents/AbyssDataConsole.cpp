// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssDataConsole.h"
#include "AbyssDiverCharacter.h"
#include "AbyssGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AAbyssDataConsole::AAbyssDataConsole()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
}

// Called when the game starts or when spawned
void AAbyssDataConsole::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AAbyssDataConsole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	if (!bIsWorking || bCompleted || !WorkingCharacter)
	{
		return;
	}

	if (!CanWork(WorkingCharacter))
	{
		StopDownload(WorkingCharacter);
		return;
	}

	CurrentDownloadTime += DeltaTime;

	const float Progress = FMath::Clamp(CurrentDownloadTime / DownloadTime, 0.0f, 1.0f);

	WorkingCharacter->Client_UpdateWorkProgress(Progress);

	if (CurrentDownloadTime >= DownloadTime)
	{
		CompleteDownload();
	}
}

bool AAbyssDataConsole::CanWork(AAbyssDiverCharacter* Character) const
{
	if (!Character)
	{
		return false;
	}

	if (bCompleted)
	{
		return false;
	}

	
	if (!Character->IsHoldingUSBItem())
	{
		return false;
	}
	

	const float Distance = FVector::Dist(Character->GetActorLocation(), GetActorLocation());

	if (Distance > MaxWorkDistance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataConsole] CanWork Failed: Too Far"));
		return false;
	}

	return true;
}

void AAbyssDataConsole::StartDownload(AAbyssDiverCharacter* Character)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataConsole] No Authority"));
		return;
	}

	if (bIsWorking || bCompleted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataConsole] Already Working or Completed"));
		return;
	}

	if (!CanWork(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataConsole] CanWork Failed"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[DataConsole] Download Start"));

	WorkingCharacter = Character;
	bIsWorking = true;
	CurrentDownloadTime = 0.0f;

	WorkingCharacter->Client_SetWorkInputBlocked(true);
	WorkingCharacter->Client_ShowWorkUI();
	WorkingCharacter->Client_UpdateWorkProgress(0.0f);
}

void AAbyssDataConsole::StopDownload(AAbyssDiverCharacter* Character)
{
	if (!HasAuthority())
	{
		return;
	}

	if (WorkingCharacter != Character)
	{
		return;
	}

	if (WorkingCharacter)
	{
		WorkingCharacter->Client_SetWorkInputBlocked(false);
		WorkingCharacter->Client_UpdateWorkProgress(0.0f);
		WorkingCharacter->Client_HideWorkUI();
	}

	WorkingCharacter = nullptr;
	bIsWorking = false;
	CurrentDownloadTime = 0.0f;
}

void AAbyssDataConsole::CompleteDownload()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!WorkingCharacter)
	{
		return;
	}

	bCompleted = true;
	bIsWorking = false;

	WorkingCharacter->Client_SetWorkInputBlocked(false);
	WorkingCharacter->Client_UpdateWorkProgress(1.0f);
	WorkingCharacter->Client_HideWorkUI();

	// 여기서 미션 진행도 +1
	// 네 현재 미션 함수 이름에 맞게 연결
	if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
	{
		GM->AddMissionProgressById(MissionId, 1);
	}

	UE_LOG(LogTemp, Warning, TEXT("[DataConsole] Download Complete"));

	WorkingCharacter = nullptr;
}

void AAbyssDataConsole::OnRep_Completed()
{
	// 상호작용
}

void AAbyssDataConsole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssDataConsole, bCompleted);
	DOREPLIFETIME(AAbyssDataConsole, bIsWorking);
	DOREPLIFETIME(AAbyssDataConsole, CurrentDownloadTime);
}