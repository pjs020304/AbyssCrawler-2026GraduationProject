// Fill out your copyright notice in the Description page of Project Settings.


#include "Mission/Contents/AbyssMissionItem.h"
#include "AbyssGameMode.h"
#include "AbyssDiverCharacter.h"
#include "AbyssGameState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Mission/UI/MissionInteractPromptWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AAbyssMissionItem::AAbyssMissionItem()
{
    bReplicates = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    MissionPromptWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("MissionPromptWidgetComp"));
    MissionPromptWidgetComp->SetupAttachment(RootComponent);
    MissionPromptWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
    MissionPromptWidgetComp->SetDrawAtDesiredSize(true);
    MissionPromptWidgetComp->SetDrawSize(FVector2D(300.0f, 120.0f));
    MissionPromptWidgetComp->SetRelativeLocation(PromptWidgetOffset);
    MissionPromptWidgetComp->SetPivot(FVector2D(0.5f, 0.0f));
    MissionPromptWidgetComp->SetVisibility(false);
    MissionPromptWidgetComp->SetHiddenInGame(true);
}

void AAbyssMissionItem::Interact_Implementation(AActor* InstigatorActor)
{
    if (!HasAuthority()) return;

    AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
    if (!Diver) return;

    AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
    if (!GS) return;

    // 해당 미션을 안 받았으면 작동 X
    if (!GS->HasActiveMission(MissionId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionItem] Mission not active: %s"), *MissionId.ToString());
        return;
    }

    if (PickupSound)
    {
        Diver->Client_PlaySound2D(PickupSound);
    }

    if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
    {
        GM->AddMissionProgressById(MissionId, 1);
    }

    if (bDestroyOnCollect)
    {
        Destroy();
    }

}

void AAbyssMissionItem::OnFocus_Implementation()
{
    if (MissionPromptWidgetComp)
    {
        MissionPromptWidgetComp->SetRelativeLocation(PromptWidgetOffset);
        MissionPromptWidgetComp->SetVisibility(true);
        MissionPromptWidgetComp->SetHiddenInGame(false);
    }

    UpdateMissionPromptUI();
}

void AAbyssMissionItem::OnLostFocus_Implementation()
{
    if (MissionPromptWidgetComp)
    {
        MissionPromptWidgetComp->SetVisibility(false);
        MissionPromptWidgetComp->SetHiddenInGame(true);
    }
}

void AAbyssMissionItem::UpdateMissionPromptUI()
{
    if (!MissionPromptWidgetComp)
    {
        return;
    }

    MissionPromptWidgetComp->InitWidget();

    UMissionInteractPromptWidget* PromptWidget =
        Cast<UMissionInteractPromptWidget>(MissionPromptWidgetComp->GetUserWidgetObject());

    if (!PromptWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionItemPrompt] Cast Failed or Widget Null: %s"), *GetName());
        return;
    }

    const bool bHasMission = IsMissionActiveForPrompt();

    const FText DisplayName = !ItemDisplayName.IsEmpty()
        ? ItemDisplayName
        : PromptObjectName;

    const FText StateText = bHasMission
        ? ActiveStateText
        : InactiveStateText;

    PromptWidget->UpdateMissionPrompt(
        DisplayName,
        StateText,
        bHasMission
    );
}

bool AAbyssMissionItem::IsMissionActiveForPrompt() const
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