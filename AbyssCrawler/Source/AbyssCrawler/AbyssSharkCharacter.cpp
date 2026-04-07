#include "AbyssSharkCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"
#include "AIController.h"

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
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent")); // ASC 컴포넌트 생성 및 기본 세팅
	// 서버에서만 권한을 가지도록 설정
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);// AI는 최적화를 위해 Minimal 모드 사용 

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

	// AI의 GAS 초기화 (서버/자신에게 권한 부여)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AAbyssSharkCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 2. ASC 초기화 (Owner와 Avatar가 누구인지 엔진에 등록)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 3. 서버(권한이 있는 곳)에서만 어빌리티를 부여합니다.
		if (HasAuthority() && !bAbilitiesInitialized)
		{
			// 기절 태그의 추가/삭제 이벤트를 감지하도록 리스너 등록
			AbilitySystemComponent->RegisterGameplayTagEvent(StunTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &AAbyssSharkCharacter::OnStunTagChanged);

			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
				.AddUObject(this, &AAbyssSharkCharacter::OnHealthChangedCallback);

			// 배열에 세팅된 스킬들을 하나씩 꺼내서 상어에게 부여(Grant)합니다.
			for (TSubclassOf<UGameplayAbility>& StartupAbility : StartupAbilities)
			{
				if (StartupAbility)
				{
					// FGameplayAbilitySpec은 어빌리티의 실행 정보를 담는 컨테이너입니다.
					// 인자: (부여할 스킬, 레벨, 입력 키 ID(보통 AI는 INDEX_NONE), 스킬의 주인)
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
		// NewCount가 1 이상이면 태그가 존재하는 것(기절 상태), 0이면 풀린 것
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

	// 체력이 0 이하로 떨어지면 사망 처리
	if (NewHealth <= 0.0f)
	{
		Die();
	}
}

void AAbyssSharkCharacter::Die()
{
	UE_LOG(LogTemp, Warning, TEXT("상어 사망!"));

	// 1. 행동 트리 정지
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBrainComponent())
	{
		AIController->GetBrainComponent()->StopLogic("Shark Died");
	}

	// 2. 콜리전 해제 (시체에 부딪히지 않게)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 사망 애니메이션 몽타주 재생 또는 랙돌(Ragdoll) 켜기
	// PlayAnimMontage(DeathMontage);
}