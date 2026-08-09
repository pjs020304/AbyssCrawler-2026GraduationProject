#include "AbyssPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbyssDiverCharacter.h"
#include <Lobby/Contents/LobbyPlayerState.h>

AAbyssPlayerState::AAbyssPlayerState()
{
	// 멀티플레이어 게임에서 GAS는 PlayerState에 두는 것이 정석입니다.
	// (캐릭터가 죽어서 사라져도 스탯 정보는 남아야 하거나, 리스폰 시 복구하기 위해)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	bIsAlive = true;
	//NetUpdateFrequency = 100.0f; // 빠른 동기화
}

UAbilitySystemComponent* AAbyssPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAbyssPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssPlayerState, bIsAlive);
	DOREPLIFETIME(AAbyssPlayerState, Nickname);
	DOREPLIFETIME(AAbyssPlayerState, PlayerColorIndex);
}

void AAbyssPlayerState::OnRep_Nickname()
{
}

void AAbyssPlayerState::OnRep_PlayerColorIndex()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerColor] OnRep ColorIndex=%d"), PlayerColorIndex);

	if (APawn* Pawn = GetPawn())
	{
		if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(Pawn))
		{
			Diver->ApplyPlayerSuitMaterial();
		}
	}
}

void AAbyssPlayerState::SetPlayerColorIndex(int32 NewIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 MaxColorCount = 3;

	PlayerColorIndex = FMath::Clamp(NewIndex, 0, MaxColorCount - 1);

	OnRep_PlayerColorIndex();

	UE_LOG(LogTemp, Warning, TEXT("[PlayerColor] Set ColorIndex=%d"), PlayerColorIndex);
}