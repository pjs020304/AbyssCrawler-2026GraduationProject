// Fill out your copyright notice in the Description page of Project Settings.

#include "AbyssReviveStation.h"
#include "AbyssDiverCharacter.h"
#include "AbyssPlayerState.h"
#include "AbyssGameState.h"
#include "AbyssGameMode.h"
#include "AbyssCorpseItem.h"
#include "AbyssAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

AAbyssReviveStation::AAbyssReviveStation()
{
	bReplicates = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionProfileName(TEXT("BlockAllDynamic")); // 시선 트레이스에 맞아야 상호작용 가능
	RootComponent = MeshComp;

	InteractWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractWidget"));
	InteractWidget->SetupAttachment(RootComponent);
	InteractWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractWidget->SetVisibility(false);
}

void AAbyssReviveStation::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsOnCooldown) return;

	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
	if (!GS) return;

	// 죽어 있는 플레이어 수집 (bIsAlive는 서버 권한 값)
	TArray<AAbyssPlayerState*> DeadPlayers;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (AAbyssPlayerState* AbyssPS = Cast<AAbyssPlayerState>(PS))
		{
			if (!AbyssPS->bIsAlive)
			{
				DeadPlayers.Add(AbyssPS);
			}
		}
	}

	if (DeadPlayers.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Revive] 부활 대상이 없습니다"));
		return;
	}

	// 비용 검증 (전원 일괄 부활이므로 총액으로 확인)
	if (ReviveCostPerPlayer > 0)
	{
		const int32 TotalCost = ReviveCostPerPlayer * DeadPlayers.Num();
		if (!GS->ConsumeSharedMoney(TotalCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Revive] 잔액 부족: %d 필요"), TotalCost);
			return;
		}
	}

	// ① 죽은 캐릭터 껍데기를 먼저 정리 (아이템 드롭 포함)
	CleanupDeadCharacters();

	// ② 각 플레이어 부활
	int32 RevivedCount = 0;
	for (AAbyssPlayerState* DeadPS : DeadPlayers)
	{
		if (RevivePlayer(DeadPS))
		{
			++RevivedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Revive] %d명 부활 완료"), RevivedCount);

	if (RevivedCount > 0)
	{
		bIsOnCooldown = true;
		GetWorldTimerManager().SetTimer(CooldownTimerHandle, this,
			&AAbyssReviveStation::ResetCooldown, ReviveCooldown, false);
	}
}

void AAbyssReviveStation::CleanupDeadCharacters()
{
	// bIsDead는 서버에서만 신뢰 가능한 값이지만, 여기는 서버 전용 경로라 안전
	for (TActorIterator<AAbyssDiverCharacter> It(GetWorld()); It; ++It)
	{
		AAbyssDiverCharacter* Diver = *It;
		if (Diver && Diver->bIsDead)
		{
			// 들고 있던 아이템을 그 자리에 드롭 (소실 방지)
			Diver->DropAllInventoryItems();
			Diver->Destroy();
		}
	}
}

bool AAbyssReviveStation::RevivePlayer(AAbyssPlayerState* DeadPlayerState)
{
	if (!DeadPlayerState) return false;

	AController* Controller = Cast<AController>(DeadPlayerState->GetOwner());
	AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>();
	if (!Controller || !GM) return false;

	// GAS 어트리뷰트 리셋: ASC가 PlayerState 소속이라 죽은 뒤에도 체력 0이 유지되고 있다.
	// 리셋 없이 재스폰하면 스폰 즉시 재사망하므로 반드시 먼저 복구한다.
	if (UAbilitySystemComponent* ASC = DeadPlayerState->GetAbilitySystemComponent())
	{
		if (const UAbyssAttributeSet* Attr = Cast<UAbyssAttributeSet>(DeadPlayerState->GetAttributeSet()))
		{
			ASC->SetNumericAttributeBase(UAbyssAttributeSet::GetHealthAttribute(), Attr->GetMaxHealth());
			ASC->SetNumericAttributeBase(UAbyssAttributeSet::GetOxygenAttribute(), Attr->GetMaxOxygen());
		}
	}

	// 생존 플래그 복구 (전멸 판정 대상에서 제외)
	DeadPlayerState->bIsAlive = true;

	// 관전 상태 해제 (Server_Die에서 Spectating으로 보냈던 것을 역으로)
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->ChangeState(NAME_Inactive);
		PC->ClientGotoState(NAME_Inactive);
	}

	// PlayerStart 지점에 새 캐릭터 스폰 + 빙의 (엔진 표준 리스폰 경로)
	GM->RestartPlayer(Controller);

	// 부활했으니 그 플레이어의 시체는 제거
	RemoveCorpseOf(DeadPlayerState);

	return true;
}

void AAbyssReviveStation::RemoveCorpseOf(const AAbyssPlayerState* PlayerState)
{
	for (TActorIterator<AAbyssCorpseItem> It(GetWorld()); It; ++It)
	{
		AAbyssCorpseItem* Corpse = *It;
		if (!Corpse || Corpse->GetDeadPlayerState() != PlayerState) continue;

		// 누가 운반 중인 시체는 인벤토리 슬롯이 꼬이므로 건드리지 않는다
		if (Corpse->IsPickedUp()) continue;

		Corpse->Destroy();
	}
}

void AAbyssReviveStation::ResetCooldown()
{
	bIsOnCooldown = false;
}

void AAbyssReviveStation::OnFocus_Implementation()
{
	if (InteractWidget) InteractWidget->SetVisibility(true);
}

void AAbyssReviveStation::OnLostFocus_Implementation()
{
	if (InteractWidget) InteractWidget->SetVisibility(false);
}
