#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h" // GAS 연동을 위한 인터페이스
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

	// --- [GAS 필수 오버라이드] ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// AI가 이 폰에 빙의할 때 호출되는 함수 (여기서 ASC를 초기화하고 시작 어빌리티를 부여한다)
	virtual void PossessedBy(AController* NewController) override;

protected:
	virtual void BeginPlay() override;

	// --- [GAS 컴포넌트] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbilitySystemComponent* AbilitySystemComponent;



	// 게임 시작 시 이 캐릭터에게 부여할 어빌리티(스킬) 목록
	// 에디터에서 물기 공격에 해당하는 GA_SharkBite를 지정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;	

	// 어빌리티가 이미 부여되었는지 확인하는 플래그 (중복 부여 방지)
	bool bAbilitiesInitialized;

	// 기절 상태를 나타내는 태그 ("State.Debuff.Stun")
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag StunTag;

	// 태그가 추가되거나 제거될 때 호출되는 콜백 함수
	virtual void OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 플레이어와 같은 클래스를 쓰는 AttributeSet (체력 등 능력치를 담는다)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbyssAttributeSet* AttributeSet;

	// 체력이 바뀔 때 호출되는 콜백 함수
	void OnHealthChangedCallback(const FOnAttributeChangeData& Data);

	// 사망 처리 함수
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