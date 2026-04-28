#include "ShopSellZone.h"
#include "Components/BoxComponent.h"
#include "AbyssItemBase.h"
#include "AbyssGameState.h"

AShopSellZone::AShopSellZone()
{
	bReplicates = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	// 레이저에 맞아야 상호작용이 되므로 Block 설정
	BaseMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = BaseMesh;

	SellAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SellAreaBox"));
	SellAreaBox->SetupAttachment(RootComponent);
	SellAreaBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 던져진 아이템(AbyssItemBase)이 떨어졌을 때 겹침을 인식하도록 Overlap 설정
	SellAreaBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
}

void AShopSellZone::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;

	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
	if (!GS) return;

	TArray<AActor*> OverlappingItems;
	// 박스 안에 겹쳐있는 AAbyssItemBase 액터만 싹 가져옵니다.
	SellAreaBox->GetOverlappingActors(OverlappingItems, AAbyssItemBase::StaticClass());

	int32 TotalEarnedMoney = 0;

	for (AActor* Actor : OverlappingItems)
	{
		AAbyssItemBase* Item = Cast<AAbyssItemBase>(Actor);
		if (Item)
		{
			// 가격의 50% 계산
			int32 SellPrice = FMath::FloorToInt(Item->ItemPrice * 0.5f);
			TotalEarnedMoney += SellPrice;

			// 팔린 아이템 파괴
			Item->Destroy();
		}
	}

	if (TotalEarnedMoney > 0)
	{
		GS->AddSharedMoney(TotalEarnedMoney);
		UE_LOG(LogTemp, Warning, TEXT("Total %d Gold Sold!"), TotalEarnedMoney);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("This area has no items"));
	}
}

// 쳐다보면 "판매하기" 위젯을 띄우고 싶다면 여기 구현 예정
void AShopSellZone::OnFocus_Implementation() {}
void AShopSellZone::OnLostFocus_Implementation() {}