#include "AbyssSharkCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"
#include "AIController.h"

AAbyssSharkCharacter::AAbyssSharkCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. [AI ����] �ʿ� ��ġ�ǰų� ������ �� �ڵ����� AI Controller�� ����(Possess)�ϵ��� ����
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 2. [���� & �̵� ����] ���� ȯ�濡 ���� ����� ������
	if (GetCapsuleComponent())
	{
		// �����̹Ƿ� �߷��� ������ ���� �ʵ��� ����
		GetCapsuleComponent()->SetEnableGravity(false);
	}

	if (GetCharacterMovement())
	{
		// ���� ���Ʒ��� �����Ӱ� �������� �ϹǷ� ����(Flying) ��� ���
		GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);

		// ȸ�� �� �Ҷ� ������ �ʰ� �ε巴�� ��ǥ�� ���� ���� �������� ����
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 150.0f, 0.0f);

		// �ִ� ���� �ӵ� (���ϴ� �������� ���� �����ϼ���)
		GetCharacterMovement()->MaxFlySpeed = 600.0f;
		GetCharacterMovement()->BrakingDecelerationFlying = 1000.0f; // ���� ���� ������
	}

	// 3. [GAS ����] �÷��̾�� �����ϰ� �������� ���� �� �ֵ��� �ý��� ����
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent")); // ASC ������Ʈ ���� �� �⺻ ����
	// ���������� ������ �������� ����
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);// AI�� ����ȭ�� ���� Minimal ��� ��� 

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

	// AI�� GAS �ʱ�ȭ (����/�ڽſ��� ���� �ο�)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AAbyssSharkCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 2. ASC �ʱ�ȭ (Owner�� Avatar�� �������� ������ ���)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 3. ����(������ �ִ� ��)������ �����Ƽ�� �ο��մϴ�.
		if (HasAuthority() && !bAbilitiesInitialized)
		{
			// ���� �±��� �߰�/���� �̺�Ʈ�� �����ϵ��� ������ ���
			AbilitySystemComponent->RegisterGameplayTagEvent(StunTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &AAbyssSharkCharacter::OnStunTagChanged);

			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
				.AddUObject(this, &AAbyssSharkCharacter::OnHealthChangedCallback);

			// �迭�� ���õ� ��ų���� �ϳ��� ������ ���� �ο�(Grant)�մϴ�.
			for (TSubclassOf<UGameplayAbility>& StartupAbility : StartupAbilities)
			{
				if (StartupAbility)
				{
					// FGameplayAbilitySpec�� �����Ƽ�� ���� ������ ��� �����̳��Դϴ�.
					// ����: (�ο��� ��ų, ����, �Է� Ű ID(���� AI�� INDEX_NONE), ��ų�� ����)
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
		// NewCount�� 1 �̻��̸� �±װ� �����ϴ� ��(���� ����), 0�̸� Ǯ�� ��
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

	// ü���� 0 ���Ϸ� �������� ��� ó��
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

	UE_LOG(LogTemp, Warning, TEXT("��� ���!"));

	// 1. �ൿ Ʈ�� ����
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBrainComponent())
	{
		AIController->GetBrainComponent()->StopLogic("Shark Died");
	}

	// 2. �ݸ��� ���� (��ü�� �ε����� �ʰ�)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 이동 정지 (사망 애니메이션 재생 중 미끄러짐 방지)
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	// 3. ��� �ִϸ��̼� ��Ÿ�� ��� �Ǵ� ����(Ragdoll) �ѱ�
	// PlayAnimMontage(DeathMontage);
}