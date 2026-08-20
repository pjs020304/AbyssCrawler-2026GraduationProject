#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "ShopItemDisplay.generated.h"

class UWidgetComponent;
class UDataTable;
class AAbyssItemBase;

/**
 * 월드 진열대 상점 (옵션 A).
 *  - 상품은 DataTable(FAbyssShopItemRow)에서 가중치 랜덤으로 선택 (미지정 시 레거시 ItemPool 사용)
 *  - 구매 후 파괴되지 않고 RestockDelay 뒤 재입고
 *  - MaxPurchaseCount 회 판매 후 매진 처리 (-1 = 무제한)
 *  - 모든 구매 검증은 서버에서 수행 (SharedMoney 차감/환불)
 */
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

	// [서버] 상품 테이블(또는 레거시 ItemPool)에서 가중치 랜덤으로 진열 상품 선택
	void SelectRandomItem();

	// [서버] 구매 직후 재입고 대기 시작
	void BeginRestock();

	// [서버] 재입고 완료: 새 상품 진열
	void FinishRestock();

	// 진열 상태(상품/재입고/매진)에 맞춰 메시 갱신
	void UpdateVisuals();

	// 진열 상태가 바뀔 때마다 호출되는 BP 훅.
	// 가격표 위젯 텍스트 갱신, 재입고 연출 등을 블루프린트에서 구현한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnDisplayUpdated();

	// 진열 상태 리플리케이션 수신 시 비주얼 갱신 (클라이언트)
	UFUNCTION()
	void OnRep_ShopState();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* PriceWidget;

	// ── 상품 정의 ─────────────────────────────────────────────
	// 상품 데이터 테이블 (행 구조: FAbyssShopItemRow). 지정 시 ItemPool보다 우선.
	UPROPERTY(EditAnywhere, Category = "Shop", meta = (RequiredAssetDataTags = "RowStructure=/Script/AbyssCrawler.AbyssShopItemRow"))
	TObjectPtr<UDataTable> ShopItemTable;

	// (레거시) 테이블 미지정 시 사용하는 후보 배열. 모두 동일 가중치.
	UPROPERTY(EditAnywhere, Category = "Shop")
	TArray<TSubclassOf<AAbyssItemBase>> ItemPool;

	// ── 재고/재입고 ───────────────────────────────────────────
	// 총 판매 가능 횟수. -1 = 무제한
	UPROPERTY(EditAnywhere, Category = "Shop|Stock")
	int32 MaxPurchaseCount = -1;

	// 구매 후 다음 상품이 진열되기까지의 시간(초). 0 이하면 즉시 재입고
	UPROPERTY(EditAnywhere, Category = "Shop|Stock")
	float RestockDelay = 5.0f;

	// true면 재입고 시 같은 상품 유지, false면 새로 랜덤 선택
	UPROPERTY(EditAnywhere, Category = "Shop|Stock")
	bool bKeepSameItemOnRestock = false;

	// ── 리플리케이트 상태 ─────────────────────────────────────
	UPROPERTY(ReplicatedUsing = OnRep_ShopState, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<AAbyssItemBase> SelectedItemClass;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shop")
	int32 SelectedItemPrice = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shop")
	FString SelectedItemName;

	// 재입고 대기 중 (구매 불가, 메시 숨김)
	UPROPERTY(ReplicatedUsing = OnRep_ShopState, BlueprintReadOnly, Category = "Shop")
	bool bIsRestocking = false;

	// 매진 (더 이상 판매 안 함)
	UPROPERTY(ReplicatedUsing = OnRep_ShopState, BlueprintReadOnly, Category = "Shop")
	bool bSoldOut = false;

private:
	// [서버 전용] 지금까지 판매된 횟수
	int32 PurchasedCount = 0;

	FTimerHandle RestockTimerHandle;
};
