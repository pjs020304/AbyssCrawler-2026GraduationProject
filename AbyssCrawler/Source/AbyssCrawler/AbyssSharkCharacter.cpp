#include "AbyssSharkCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"

AAbyssSharkCharacter::AAbyssSharkCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. [AI 설정] 맵에 배치되거나 스폰될 때 자동으로 AI Controller가 빙의(Possess)하도록 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 2. [물리 & 이동 설정] 심해 환경에 맞춘 상어의 움직임
	if (GetCapsuleComponent())
	{
		// 물속이므로 중력의 영향을 받지 않도록 설정
		GetCapsuleComponent()->SetEnableGravity(false);
	}

	if (GetCharacterMovement())
	{
		// 상어는 위아래로 자유롭게 움직여야 하므로 비행(Flying) 모드 사용
		GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);

		// 회전 시 뚝뚝 끊기지 않고 부드럽게 목표를 향해 몸을 돌리도록 설정
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 150.0f, 0.0f);

		// 최대 수영 속도 (원하는 공포감에 맞춰 조절하세요)
		GetCharacterMovement()->MaxFlySpeed = 600.0f;
		GetCharacterMovement()->BrakingDecelerationFlying = 1000.0f; // 멈출 때의 마찰력
	}

	// 3. [GAS 설정] 플레이어와 동일하게 데미지를 받을 수 있도록 시스템 부착
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal); // AI는 최적화를 위해 Minimal 모드 사용 

	AttributeSet = CreateDefaultSubobject<UAbyssAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AAbyssSharkCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAbyssSharkCharacter::BeginPlay()
{
	Super::BeginPlay();

	// AI의 GAS 초기화 (서버/자신에게 권한 부여)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}