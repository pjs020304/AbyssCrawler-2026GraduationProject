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
	// [주의] 실제 게임플레이 어트리뷰트(체력/산소/배터리)는 여기가 아니라
	// AAbyssDiverCharacter가 소유한 ASC + UAbyssAttributeSet에 들어 있다.
	// 아래 AttributeSet은 생성자에서 만들어지지 않아 항상 nullptr이므로,
	// 스탯을 읽거나 쓰려면 반드시 캐릭터 쪽 ASC를 사용할 것.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	// --- Custom State ---
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abyss State")
	bool bIsAlive; // 생존 여부 (사망 시 관전 모드 전환용)

	UPROPERTY(ReplicatedUsing = OnRep_Nickname, EditAnywhere, BlueprintReadWrite)
	FText Nickname = FText::FromString(TEXT("Player"));

	UFUNCTION()
	void OnRep_Nickname();

	UPROPERTY(ReplicatedUsing = OnRep_PlayerColorIndex, BlueprintReadOnly, Category = "Player|Color")
	int32 PlayerColorIndex = 0;

	UFUNCTION()
	void OnRep_PlayerColorIndex();

	void SetPlayerColorIndex(int32 NewIndex);

protected:
	// GAS 컴포넌트 (Replicated)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abyss GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	// 스탯 (체력, 산소, 배터리) 정의
	UPROPERTY()
	UAttributeSet* AttributeSet;
};