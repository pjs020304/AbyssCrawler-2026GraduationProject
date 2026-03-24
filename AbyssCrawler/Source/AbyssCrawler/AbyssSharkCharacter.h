#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h" // GAS 연동용 인터페이스
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

protected:
	virtual void BeginPlay() override;

	// --- [GAS 컴포넌트] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UAbyssAttributeSet* AttributeSet;
};