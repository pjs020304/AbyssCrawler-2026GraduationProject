#include "AbyssPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

AAbyssPlayerState::AAbyssPlayerState()
{
	// 멀티플레이어 게임에서 GAS는 PlayerState에 두는 것이 정석입니다.
	// (캐릭터가 죽어서 사라져도 스탯 정보는 남아야 하거나, 리스폰 시 복구하기 위해)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	bIsAlive = true;
	NetUpdateFrequency = 100.0f; // 빠른 동기화
}

UAbilitySystemComponent* AAbyssPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAbyssPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssPlayerState, bIsAlive);
}