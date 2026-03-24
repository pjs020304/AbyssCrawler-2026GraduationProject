#include "AbyssAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UAbyssAttributeSet::UAbyssAttributeSet()
{
	// 기본값 초기화
	InitHealth(100.0f);
	InitMaxHealth(100.0f);

	InitOxygen(100.0f);
	InitMaxOxygen(100.0f);

	InitBattery(100.0f);
	InitMaxBattery(100.0f);
}

// 서버의 데이터를 클라이언트와 동기화하는 설정입니다.
void UAbyssAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UAbyssAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAbyssAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UAbyssAttributeSet, Oxygen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAbyssAttributeSet, MaxOxygen, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UAbyssAttributeSet, Battery, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAbyssAttributeSet, MaxBattery, COND_None, REPNOTIFY_Always);
}

// [핵심 로직] 값이 실제로 적용되기 직전에 검사하여 안전하게 다듬습니다.
void UAbyssAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 체력이 변경될 때: 0과 최대 체력 사이로 고정
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	// 산소가 변경될 때: 0과 최대 산소 사이로 고정
	else if (Attribute == GetOxygenAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxOxygen());
	}
	// 배터리가 변경될 때: 0과 최대 배터리 사이로 고정
	else if (Attribute == GetBatteryAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxBattery());
	}
}

// 이펙트 처리가 끝난 후 데미지 연산 등을 추가할 공간입니다.
void UAbyssAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 추후 몬스터 타격 데미지 처리를 이곳에서 구현하게 됩니다.
}

// --- 아래는 리플리케이션(동기화) 필수 함수들입니다 ---
void UAbyssAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAbyssAttributeSet, Health, OldHealth);
}

void UAbyssAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAbyssAttributeSet, MaxHealth, OldMaxHealth);
}

void UAbyssAttributeSet::OnRep_Oxygen(const FGameplayAttributeData& OldOxygen)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAbyssAttributeSet, Oxygen, OldOxygen);
}

void UAbyssAttributeSet::OnRep_MaxOxygen(const FGameplayAttributeData& OldMaxOxygen)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAbyssAttributeSet, MaxOxygen, OldMaxOxygen);
}

void UAbyssAttributeSet::OnRep_Battery(const FGameplayAttributeData& OldBattery)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAbyssAttributeSet, Battery, OldBattery);
}

void UAbyssAttributeSet::OnRep_MaxBattery(const FGameplayAttributeData& OldMaxBattery)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAbyssAttributeSet, MaxBattery, OldMaxBattery);
}