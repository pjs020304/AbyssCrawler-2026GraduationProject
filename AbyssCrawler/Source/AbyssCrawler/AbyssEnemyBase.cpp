#include "AbyssEnemyBase.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h" // 프로젝트에 맞게 헤더명 확인
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"

AAbyssEnemyBase::AAbyssEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bAbilitiesInitialized = false;

	// 1. ASC 컴포넌트 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// 2. 어트리뷰트 셋(체력 등) 생성
	AttributeSet = CreateDefaultSubobject<UAbyssAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AAbyssEnemyBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAbyssEnemyBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 기본 체력 초기화
		if (AttributeSet)
		{
			AttributeSet->InitHealth(100.0f);
		}

		// 체력 및 기절 태그 리스너 바인딩
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddUObject(this, &AAbyssEnemyBase::OnHealthChangedCallback);

		AbilitySystemComponent->RegisterGameplayTagEvent(StunTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AAbyssEnemyBase::OnStunTagChanged);

		// 어빌리티 부여 (서버 전용)
		if (HasAuthority() && !bAbilitiesInitialized)
		{
			for (TSubclassOf<UGameplayAbility>& StartupAbility : StartupAbilities)
			{
				if (StartupAbility)
				{
					AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, 1, INDEX_NONE, this));
				}
			}
			bAbilitiesInitialized = true;
		}
	}
}

void AAbyssEnemyBase::OnHealthChangedCallback(const FOnAttributeChangeData& Data)
{
	float NewHealth = Data.NewValue;
	if (NewHealth <= 0.0f)
	{
		Die();
	}
}

void AAbyssEnemyBase::OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBlackboardComponent())
	{
		bool bIsStunned = (NewCount > 0);
		AIController->GetBlackboardComponent()->SetValueAsBool(TEXT("IsStunned"), bIsStunned);
	}
}

void AAbyssEnemyBase::Die()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBrainComponent())
	{
		// 행동 트리 즉시 정지
		AIController->GetBrainComponent()->StopLogic("Enemy Died");
	}

	// 콜리전 해제
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UE_LOG(LogTemp, Warning, TEXT("[%s] 사망했습니다."), *GetName());
}