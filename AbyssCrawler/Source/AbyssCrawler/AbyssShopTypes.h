// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AbyssShopTypes.generated.h"

class AAbyssItemBase;

/**
 * 상점 상품 정의 (DataTable 행).
 * 진열대(ShopItemDisplay)와 판매존(ShopSellZone)이 공용으로 사용한다.
 * 상품 추가/밸런싱은 코드 수정 없이 DataTable 행 편집으로 끝낸다.
 */
USTRUCT(BlueprintType)
struct FAbyssShopItemRow : public FTableRowBase
{
	GENERATED_BODY()

	// 판매할 아이템 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TSubclassOf<AAbyssItemBase> ItemClass;

	// 진열 확률 가중치 (높을수록 진열대에 자주 등장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	float Weight = 1.0f;

	// 구매 가격 재정의. 0 이하면 아이템의 기본 ItemPrice 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	int32 PriceOverride = 0;

	// 판매 시세 배율 (SellZone용, 옵션 C에서 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	float SellPriceMultiplier = 0.5f;
};
