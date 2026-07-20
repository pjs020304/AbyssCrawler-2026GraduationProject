#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h" // GAS ������ �������̽�
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "AbyssSharkCharacter.generated.h"

class UAbilitySystemComponent;
class UAbyssAttributeSet;

UCLASS()
class ABYSSCRAWLER_API AAbyssSharkCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAbyssSharkCharacter();

	// --- [GAS �ʼ� �������̵�] ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// AI�� ������ �� ȣ��Ǵ� �Լ� (���⼭ ASC �ʱ�ȭ �� ��ų �ο��� �����մϴ�)
	virtual void PossessedBy(AController* NewController) override;

protected:
	virtual void BeginPlay() override;

	// --- [GAS ������Ʈ] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbilitySystemComponent* AbilitySystemComponent;



	// ���� ���� �� ���� �ο��� �����Ƽ(��ų) ���
	// �����Ϳ��� ���⿡ ��� ���� GA_SharkBite�� ����
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;	

	// �����Ƽ�� �� ���� �ο��ǵ��� üũ�ϴ� �÷���
	bool bAbilitiesInitialized;

	// ���� �±� ("State.Debuff.Stun")
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag StunTag;

	// �±װ� �߰��ǰų� ������ �� ȣ��� �ݹ� �Լ�
	virtual void OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// �÷��̾�� ������ AttributeSet ���
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbyssAttributeSet* AttributeSet;

	// ü���� ���� �� ȣ��� �ݹ� �Լ�
	void OnHealthChangedCallback(const FOnAttributeChangeData& Data);

	// ��� ó�� �Լ�
	void Die();

	// 공격 어빌리티(GA_SharkBite)가 활성화된 동안 ASC에 부여되는 태그 (예: "State.Attacking")
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag AttackingTag;

	// 사망 여부 (Die()에서 true로 설정, AnimBP 트랜지션에서 사용)
	bool bIsDead = false;

public:
	// AnimInstance / Transition Rule에서 조회하는 상태 함수들
	UFUNCTION(BlueprintPure, Category = "Animation")
	bool IsAttacking() const;

	UFUNCTION(BlueprintPure, Category = "Animation")
	bool IsDead() const { return bIsDead; }
};