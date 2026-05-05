#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "ShopItemDisplay.generated.h"

class UWidgetComponent;
class AAbyssItemBase;

UCLASS()
class ABYSSCRAWLER_API AShopItemDisplay : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()

public:
	AShopItemDisplay();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

protected:
	virtual void BeginPlay() override;

	// 업데이트 함수 (mesh 및 가격표 갱신)
	void UpdateVisuals();

	// 선택된 아이템이 복제될 때 클라이언트에서 호출될 함수
	UFUNCTION()
	void OnRep_SelectedItemClass();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* PriceWidget;

	// 블루프린트에서 설정할 아이템 후보군 배열
	UPROPERTY(EditAnywhere, Category = "Shop")
	TArray<TSubclassOf<AAbyssItemBase>> ItemPool;

	// 서버에서 선택된 아이템 
	UPROPERTY(ReplicatedUsing = OnRep_SelectedItemClass, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<AAbyssItemBase> SelectedItemClass;

	// 선택된 아이템의 가격
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shop")
	int32 SelectedItemPrice;

	// 선택된 아이템의 이름	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shop")
	FString SelectedItemName;
};