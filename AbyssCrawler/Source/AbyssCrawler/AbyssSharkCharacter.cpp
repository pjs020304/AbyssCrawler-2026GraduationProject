#include "AbyssSharkCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"
#include "AIController.h"

AAbyssSharkCharacter::AAbyssSharkCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. [AI 설정] 레벨에 배치되거나 스폰될 때 자동으로 AI Controller가 빙의(Possess)하도록 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 2. [물리 & 이동 설정] 수중 환경에 맞게 조정한다
	if (GetCapsuleComponent())
	{
		// 물속이므로 중력의 영향을 받지 않도록 끈다
		GetCapsuleComponent()->SetEnableGravity(false);
	}

	if (GetCharacterMovement())
	{
		// 수중을 상하좌우 자유롭게 헤엄쳐야 하므로 비행(Flying) 모드를 쓴다
		GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);

		// 방향을 급하게 꺾지 않고 부드럽게 목표 쪽으로 돌도록 이동 방향 정렬을 켠다
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 150.0f, 0.0f);

		// 최대 유영 속도 (기획 수치에 맞춰 조절)
		GetCharacterMovement()->MaxFlySpeed = 600.0f;
		GetCharacterMovement()->BrakingDecelerationFlying = 1000.0f; // 멈출 때의 감속량
	}

	// 3. [GAS 설정] 플레이어와 동일하게 어빌리티를 쓸 수 있도록 시스템을 붙인다
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent")); // ASC 컴포넌트 생성
	// 멀티플레이 동기화를 위해 리플리케이션을 켠다
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);// AI는 최소한만 보내면 되므로 Minimal 모드

	AttributeSet = CreateDefaultSubobject<UAbyssAttributeSet>(TEXT("AttributeSet"));



	

	bAbilitiesInitialized = false;
}

UAbilitySystemComponent* AAbyssSharkCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAbyssSharkCharacter::BeginPlay()
{
	Super::BeginPlay();

	// AI의 GAS 초기화 (소유자와 아바타를 모두 자기 자신으로 지정)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AAbyssSharkCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 2. ASC 초기화 (Owner와 Avatar를 이 캐릭터 자신으로 지정)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 3. 서버(권한을 가진 쪽)에서만 시작 어빌리티를 부여한다.
		if (HasAuthority() && !bAbilitiesInitialized)
		{
			// 기절 태그가 추가되거나 제거될 때 알림을 받도록 콜백을 등록
			AbilitySystemComponent->RegisterGameplayTagEvent(StunTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &AAbyssSharkCharacter::OnStunTagChanged);

			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
				.AddUObject(this, &AAbyssSharkCharacter::OnHealthChangedCallback);

			// 배열에 지정해 둔 시작 스킬들을 하나씩 순회하며 이 캐릭터에 부여(Grant)한다.
			for (TSubclassOf<UGameplayAbility>& StartupAbility : StartupAbilities)
			{
				if (StartupAbility)
				{
					// FGameplayAbilitySpec은 어빌리티를 실제로 부여할 때 쓰는 컨테이너다.
					// 인자: (부여할 스킬, 레벨, 입력 키 ID(AI는 INDEX_NONE), 소유 액터)
					AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, 1, INDEX_NONE, this));
				}
			}

			bAbilitiesInitialized = true;
			UE_LOG(LogTemp, Warning, TEXT("Shark has Skill"));
		}
	}
}

void AAbyssSharkCharacter::OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// UE_LOG(LogTemp, Warning, TEXT("Start On Stun Tag Changed Function"));

	AAIController* AIController = Cast<AAIController>(GetController());
	
	if (AIController && AIController->GetBlackboardComponent())
	{
		// NewCount가 1 이상이면 태그가 붙어 있는 상태(기절 중), 0이면 풀린 상태
		bool bIsStunned = (NewCount > 0);
		AIController->GetBlackboardComponent()->SetValueAsBool(TEXT("IsStunned"), bIsStunned);

		if (bIsStunned)
		{
			UE_LOG(LogTemp, Warning, TEXT("Shark Stunned!!"));
		}
	}
}

void AAbyssSharkCharacter::OnHealthChangedCallback(const FOnAttributeChangeData& Data)
{
	float NewHealth = Data.NewValue;
	UE_LOG(LogTemp, Warning, TEXT("Shark Health Changed: %f"), NewHealth);

	// 체력이 0 이하로 떨어졌다면 사망 처리
	if (NewHealth <= 0.0f)
	{
		Die();
	}
}

bool AAbyssSharkCharacter::IsAttacking() const
{
	// 공격 어빌리티가 살아있는 동안 부여되는 태그로 판정
	return AbilitySystemComponent
		&& AttackingTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(AttackingTag);
}

void AAbyssSharkCharacter::Die()
{
	if (bIsDead)
	{
		return; // 중복 사망 처리 방지
	}
	bIsDead = true;

	UE_LOG(LogTemp, Warning, TEXT("Shark Died!"));

	// 1. 행동 트리 정지
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBrainComponent())
	{
		AIController->GetBrainComponent()->StopLogic("Shark Died");
	}

	// 2. 콜리전 해제 (시체에 부딪히지 않도록)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 이동 정지 (사망 애니메이션 재생 중 미끄러짐 방지)
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	// 3. 사망 애니메이션 몽타주 재생 또는 래그돌(Ragdoll) 전환
	// PlayAnimMontage(DeathMontage);
}