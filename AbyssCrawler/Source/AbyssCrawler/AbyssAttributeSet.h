#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.generated.h"

// GAS에서 필수적으로 사용하는 매크로 세트입니다. Getter/Setter 함수를 자동으로 만들어줍니다.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class ABYSSCRAWLER_API UAbyssAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAbyssAttributeSet();

	// 네트워크 멀티플레이 동기화를 위한 필수 오버라이드 함수
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// [중요] 속성값이 변경되기 직전에 호출됩니다. (최대/최소값 제한 등 클램핑 용도)
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// 게임플레이 이펙트(Damage, Heal 등)가 적용된 직후에 호출됩니다.
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

public:
	// ===========================================================
	// 1. 체력 (Health)
	// ===========================================================
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UAbyssAttributeSet, Health)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAbyssAttributeSet, MaxHealth)

		// ===========================================================
		// 2. 산소 (Oxygen)
		// ===========================================================
		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Oxygen", ReplicatedUsing = OnRep_Oxygen)
	FGameplayAttributeData Oxygen;
	ATTRIBUTE_ACCESSORS(UAbyssAttributeSet, Oxygen)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Oxygen", ReplicatedUsing = OnRep_MaxOxygen)
	FGameplayAttributeData MaxOxygen;
	ATTRIBUTE_ACCESSORS(UAbyssAttributeSet, MaxOxygen)

		// ===========================================================
		// 3. 배터리 (Battery)
		// ===========================================================
		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Battery", ReplicatedUsing = OnRep_Battery)
	FGameplayAttributeData Battery;
	ATTRIBUTE_ACCESSORS(UAbyssAttributeSet, Battery)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Battery", ReplicatedUsing = OnRep_MaxBattery)
	FGameplayAttributeData MaxBattery;
	ATTRIBUTE_ACCESSORS(UAbyssAttributeSet, MaxBattery)

protected:
	// --- 네트워크 리플리케이션(Replication) 통지 함수들 ---
	// 서버에서 값이 바뀌었을 때 클라이언트들에게 "값이 바뀌었어!" 라고 알려주는 역할입니다.

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Oxygen(const FGameplayAttributeData& OldOxygen);

	UFUNCTION()
	virtual void OnRep_MaxOxygen(const FGameplayAttributeData& OldMaxOxygen);

	UFUNCTION()
	virtual void OnRep_Battery(const FGameplayAttributeData& OldBattery);

	UFUNCTION()
	virtual void OnRep_MaxBattery(const FGameplayAttributeData& OldMaxBattery);
};