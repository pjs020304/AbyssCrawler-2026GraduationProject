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

	// ── 체력 충전: 살아 있는 사용자를 즉시 만피로. 부활 대상 유무와 무관하게 먼저 처리한다.
	// (예전에는 부활 대상이 없으면 여기서 바로 return이라 체력이 전혀 회복되지 않았다)
	bool bStationUsed = RestoreHealth(Cast<AAbyssDiverCharacter>(InstigatorActor));

	// ── 부활: 죽어 있는 팀원이 있으면 전원 일괄 부활
	int32 RevivedCount = 0;
	if (AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>())
	{
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
		}
		// 비용 검증 (전원 일괄 부활이므로 총액으로 확인). 잔액이 모자라면 부활만 건너뛴다.
		else if (ReviveCostPerPlayer > 0 &&
			!GS->ConsumeSharedMoney(ReviveCostPerPlayer * DeadPlayers.Num()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Revive] 잔액 부족: %d 필요"),
				ReviveCostPerPlayer * DeadPlayers.Num());
		}
		else
		{
			// ① 죽은 캐릭터 껍데기를 먼저 정리 (아이템 드롭 포함)
			CleanupDeadCharacters();

			// ② 각 플레이어 부활
			for (AAbyssPlayerState* DeadPS : DeadPlayers)
			{
				if (RevivePlayer(DeadPS))
				{
					++RevivedCount;
				}
			}

			UE_LOG(LogTemp, Log, TEXT("[Revive] %d명 부활 완료"), RevivedCount);
		}
	}

	bStationUsed |= (RevivedCount > 0);

	// 체력 충전이든 부활이든 실제로 뭔가 했을 때만 쿨타임 (헛손질로 잠기지 않게)
	if (bStationUsed)
	{
		bIsOnCooldown = true;
		GetWorldTimerManager().SetTimer(CooldownTimerHandle, this,
			&AAbyssReviveStation::ResetCooldown, ReviveCooldown, false);
	}
}

// 살아 있는 사용자의 체력을 최대치로 되돌린다.
// [중요] ASC와 UAbyssAttributeSet은 PlayerState가 아니라 AAbyssDiverCharacter가 소유한다.
// 산소/배터리 스테이션도 Diver->GetAbilitySystemComponent()에 GE를 걸기 때문에 동작하는 것이고,
// PlayerState의 ASC를 건드리면 어트리뷰트가 붙어 있지 않아 아무 일도 일어나지 않는다.
bool AAbyssReviveStation::RestoreHealth(AAbyssDiverCharacter* Diver)
{
	if (!Diver || Diver->bIsDead) return false;

	UAbilitySystemComponent* ASC = Diver->GetAbilitySystemComponent();
	if (!ASC) return false;

	const float MaxHealth = ASC->GetNumericAttribute(UAbyssAttributeSet::GetMaxHealthAttribute());
	const float CurHealth = ASC->GetNumericAttribute(UAbyssAttributeSet::GetHealthAttribute());

	// 이미 만피면 쿨타임만 낭비하므로 사용 실패로 취급
	if (MaxHealth <= 0.0f || CurHealth >= MaxHealth) return false;

	// SetNumericAttributeBase는 Base를 바꾸면서 어트리뷰트 변경 델리게이트도 태우므로
	// OnHealthChangedCallback → HUD 체력바까지 그대로 갱신된다.
	ASC->SetNumericAttributeBase(UAbyssAttributeSet::GetHealthAttribute(), MaxHealth);

	UE_LOG(LogTemp, Log, TEXT("[HealthStation] %s 체력 충전: %.0f -> %.0f"),
		*Diver->GetName(), CurHealth, MaxHealth);
	return true;
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

	// 새로 스폰된 캐릭터의 체력/산소를 최대치로 보정.
	// 어트리뷰트는 PlayerState가 아니라 캐릭터가 들고 있으므로 반드시 새 폰에서 가져와야 한다.
	// (예전 코드는 PlayerState의 ASC/AttributeSet을 건드렸는데, PlayerState의 AttributeSet은
	//  생성조차 되지 않아 항상 nullptr이라 이 리셋이 통째로 죽은 코드였다)
	if (AAbyssDiverCharacter* NewDiver = Cast<AAbyssDiverCharacter>(Controller->GetPawn()))
	{
		if (UAbilitySystemComponent* NewASC = NewDiver->GetAbilitySystemComponent())
		{
			NewASC->SetNumericAttributeBase(UAbyssAttributeSet::GetHealthAttribute(),
				NewASC->GetNumericAttribute(UAbyssAttributeSet::GetMaxHealthAttribute()));
			NewASC->SetNumericAttributeBase(UAbyssAttributeSet::GetOxygenAttribute(),
				NewASC->GetNumericAttribute(UAbyssAttributeSet::GetMaxOxygenAttribute()));
		}
	}

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
