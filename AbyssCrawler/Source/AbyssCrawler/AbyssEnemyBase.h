#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "AbyssEnemyBase.generated.h"

class UAbilitySystemComponent;
class UAbyssAttributeSet;
class UGameplayAbility;

UCLASS(Abstract) // [핵심] 이 클래스는 부모로만 쓰이고 직접 스폰되지 않음을 엔진에 알립니다.
class ABYSSCRAWLER_API AAbyssEnemyBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAbyssEnemyBase();

	// IAbilitySystemInterface 필수 오버라이드
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void PossessedBy(AController* NewController) override;

protected:
	// --- [GAS 공통 컴포넌트] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbyssAttributeSet* AttributeSet;

	// --- [공통 데이터 및 태그] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTag StunTag;

	bool bAbilitiesInitialized;

	// --- [공통 행동 로직] ---
	virtual void OnHealthChangedCallback(const struct FOnAttributeChangeData& Data);
	virtual void OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 사망 처리 (자식 클래스에서 오버라이드할 수 있도록 virtual 선언)
	virtual void Die();
};