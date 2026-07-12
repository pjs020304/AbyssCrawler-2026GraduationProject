#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "ShopSellZone.generated.h"

class UBoxComponent;
class UWidgetComponent;
class UDataTable;
class AAbyssItemBase;

/**
 * 판매존 (옵션 C).
 *  - 2단계 확인: 첫 상호작용 = 견적(품목 수/총액) 표시 → 제한시간 내 재상호작용 = 판매 확정
 *  - 아이템별 시세: ShopItemTable(FAbyssShopItemRow)의 SellPriceMultiplier 적용 (미등록 시 기본 배율)
 *  - 시체(AbyssCorpseItem)는 기본적으로 판매 제외 (bAllowCorpseSale)
 *  - 판매 확정 시 "견적에 포함됐던" 아이템만 판매 → 견적 표시액과 실지급액 불일치 방지
 */
UCLASS()
class ABYSSCRAWLER_API AShopSellZone : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()

public:
	AShopSellZone();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

protected:
	// [서버] 존 안의 판매 가능 아이템으로 견적 생성
	void BuildQuote();

	// [서버] 견적에 포함된 아이템 판매 확정
	void ExecuteSale();

	// [서버] 견적 취소 (시간 초과 등)
	void CancelQuote();

	// 아이템 한 개의 판매가 계산 (테이블 시세 배율 적용)
	int32 CalculateSellPrice(const AAbyssItemBase* Item) const;

	// 이 아이템을 팔 수 있는가 (픽업 상태/시체 제외 등)
	bool IsSellable(const AAbyssItemBase* Item) const;

	// 견적 상태가 바뀔 때 호출되는 BP 훅.
	// InfoWidget 텍스트 갱신용: bQuotePending / PendingSaleCount / PendingSaleTotal 을 읽어 표시한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnSellZoneUpdated();

	UFUNCTION()
	void OnRep_QuoteState();

	// ── 컴포넌트 ─────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* SellAreaBox;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* BaseMesh;

	// 견적/안내 표시용 위젯 (BP에서 위젯 클래스 지정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* InfoWidget;

	// ── 설정 ─────────────────────────────────────────────────
	// 시세 테이블 (FAbyssShopItemRow.SellPriceMultiplier 사용). 미지정/미등록 아이템은 기본 배율.
	UPROPERTY(EditAnywhere, Category = "Shop", meta = (RequiredAssetDataTags = "RowStructure=/Script/AbyssCrawler.AbyssShopItemRow"))
	TObjectPtr<UDataTable> ShopItemTable;

	// 테이블에 없는 아이템의 기본 판매 배율
	UPROPERTY(EditAnywhere, Category = "Shop")
	float DefaultSellMultiplier = 0.5f;

	// 동료 시체 판매 허용 여부
	UPROPERTY(EditAnywhere, Category = "Shop")
	bool bAllowCorpseSale = false;

	// 견적 유지 시간(초). 초과 시 자동 취소
	UPROPERTY(EditAnywhere, Category = "Shop")
	float QuoteTimeout = 8.0f;

	// ── 리플리케이트 견적 상태 (전 클라 위젯 표시용) ──────────
	UPROPERTY(ReplicatedUsing = OnRep_QuoteState, BlueprintReadOnly, Category = "Shop")
	bool bQuotePending = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shop")
	int32 PendingSaleCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shop")
	int32 PendingSaleTotal = 0;

private:
	// [서버 전용] 견적에 포함된 아이템들 (확정 시 이것만 판매)
	TArray<TWeakObjectPtr<AAbyssItemBase>> QuotedItems;

	FTimerHandle QuoteTimeoutHandle;
};
