#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "ShopSellZone.generated.h"

class UBoxComponent;

UCLASS()
class ABYSSCRAWLER_API AShopSellZone : public AActor, public IAbyssInteractionInterface 
{
	GENERATED_BODY()

public:
	AShopSellZone();

	// 인터페이스 함수 오버라이드
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* SellAreaBox;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* BaseMesh;
};