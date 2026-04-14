#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h" // GAS 연동용 인터페이스
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

	// AI가 빙의할 때 호출되는 함수 (여기서 ASC 초기화 및 스킬 부여를 진행합니다)
	virtual void PossessedBy(AController* NewController) override;

protected:
	virtual void BeginPlay() override;

	// --- [GAS 컴포넌트] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbilitySystemComponent* AbilitySystemComponent;



	// 게임 시작 시 상어에게 부여할 어빌리티(스킬) 목록
	// 에디터에서 여기에 방금 만든 GA_SharkBite를 삽입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;	

	// 어빌리티가 한 번만 부여되도록 체크하는 플래그
	bool bAbilitiesInitialized;

	// 기절 태그 ("State.Debuff.Stun")
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FGameplayTag StunTag;

	// 태그가 추가되거나 지워질 때 호출될 콜백 함수
	virtual void OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 플레이어와 동일한 AttributeSet 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbyssAttributeSet* AttributeSet;

	// 체력이 변할 때 호출될 콜백 함수
	void OnHealthChangedCallback(const FOnAttributeChangeData& Data);

	// 사망 처리 함수
	void Die();
};