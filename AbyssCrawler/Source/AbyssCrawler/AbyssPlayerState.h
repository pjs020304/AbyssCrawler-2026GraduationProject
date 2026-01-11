#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h" // GAS 필수 헤더
#include "AbyssPlayerState.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class ABYSSCRAWLER_API AAbyssPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAbyssPlayerState();

	// --- GAS Interface ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	// --- Custom State ---
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss State")
	bool bIsAlive; // 생존 여부 (사망 시 관전 모드 전환용)

protected:
	// GAS 컴포넌트 (Replicated)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	// 스탯 (체력, 산소, 배터리) 정의
	UPROPERTY()
	UAttributeSet* AttributeSet;
};