
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AbyssInteractionInterface.generated.h"

UINTERFACE(MinimalAPI)
class UAbyssInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ABYSSCRAWLER_API IAbyssInteractionInterface
{
	GENERATED_BODY()

public:

	// BlueprintNativeEvent: C++에서 기본 로직을 구현하고, 블루프린트에서 확장(Override) 가능

	// 상호작용 실행 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* InstigatorActor);

	// 플레이어의 시선이 닿았을 때
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnFocus();

	// 플레이어의 시선이 벗어났을 때
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnLostFocus();
};
