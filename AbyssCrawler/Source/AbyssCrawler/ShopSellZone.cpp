#include "ShopSellZone.h"
#include "AbyssShopTypes.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "AbyssItemBase.h"
#include "AbyssCorpseItem.h"
#include "AbyssGameState.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AShopSellZone::AShopSellZone()
{
	bReplicates = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	// 플레이어가 올라설 수 있어야 상호작용이 되므로 Block으로 설정
	BaseMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = BaseMesh;

	SellAreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SellAreaBox"));
	SellAreaBox->SetupAttachment(RootComponent);
	SellAreaBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 판매대 위의 아이템(AbyssItemBase)이 영역 안에 들어왔는지 인식하도록 Overlap으로 설정
	SellAreaBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	InfoWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InfoWidget"));
	InfoWidget->SetupAttachment(RootComponent);
	InfoWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InfoWidget->SetDrawSize(FVector2D(300.0f, 120.0f));
	InfoWidget->SetVisibility(false);
}

void AShopSellZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShopSellZone, bQuotePending);
	DOREPLIFETIME(AShopSellZone, PendingSaleCount);
	DOREPLIFETIME(AShopSellZone, PendingSaleTotal);
}

void AShopSellZone::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;

	// 1차 상호작용 = 견적 생성, 2차(제한시간 내) = 판매 확정
	if (!bQuotePending)
	{
		BuildQuote();
	}
	else
	{
		ExecuteSale();
	}
}

// [서버] 존 안의 판매 가능 아이템으로 견적 생성
void AShopSellZone::BuildQuote()
{
	QuotedItems.Reset();
	int32 Total = 0;

	TArray<AActor*> OverlappingItems;
	SellAreaBox->GetOverlappingActors(OverlappingItems, AAbyssItemBase::StaticClass());

	for (AActor* Actor : OverlappingItems)
	{
		AAbyssItemBase* Item = Cast<AAbyssItemBase>(Actor);
		if (Item && IsSellable(Item))
		{
			QuotedItems.Add(Item);
			Total += CalculateSellPrice(Item);
		}
	}

	if (QuotedItems.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SellZone] 팔 수 있는 아이템이 없습니다"));
		return;
	}

	bQuotePending = true;
	PendingSaleCount = QuotedItems.Num();
	PendingSaleTotal = Total;
	OnRep_QuoteState(); // 리슨서버 호스트 즉시 갱신

	// 제한시간 내 확정 없으면 자동 취소
	GetWorldTimerManager().SetTimer(QuoteTimeoutHandle, this,
		&AShopSellZone::CancelQuote, QuoteTimeout, false);

	UE_LOG(LogTemp, Log, TEXT("[SellZone] 견적: %d개 / 총 %d G (제한 %.0f초)"),
		PendingSaleCount, PendingSaleTotal, QuoteTimeout);
}

// [서버] 견적에 포함된 아이템만 판매 확정
void AShopSellZone::ExecuteSale()
{
	GetWorldTimerManager().ClearTimer(QuoteTimeoutHandle);

	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
	if (!GS)
	{
		CancelQuote();
		return;
	}

	int32 TotalEarned = 0;
	int32 SoldCount = 0;

	for (const TWeakObjectPtr<AAbyssItemBase>& WeakItem : QuotedItems)
	{
		AAbyssItemBase* Item = WeakItem.Get();
		// 견적 이후 사라졌거나(줍기/파괴) 존 밖으로 나간 아이템은 제외
		if (!Item || !IsSellable(Item) || !SellAreaBox->IsOverlappingActor(Item))
		{
			continue;
		}

		TotalEarned += CalculateSellPrice(Item);
		++SoldCount;
		Item->Destroy();
	}

	if (TotalEarned > 0)
	{
		GS->AddSharedMoney(TotalEarned);
		UE_LOG(LogTemp, Log, TEXT("[SellZone] 판매 확정: %d개 / +%d G"), SoldCount, TotalEarned);
	}

	CancelQuote(); // 견적 상태 초기화 (위젯 갱신 포함)
}

// [서버] 견적 취소/초기화
void AShopSellZone::CancelQuote()
{
	GetWorldTimerManager().ClearTimer(QuoteTimeoutHandle);
	QuotedItems.Reset();

	bQuotePending = false;
	PendingSaleCount = 0;
	PendingSaleTotal = 0;
	OnRep_QuoteState(); // 리슨서버 호스트 즉시 갱신
}

int32 AShopSellZone::CalculateSellPrice(const AAbyssItemBase* Item) const
{
	if (!Item) return 0;

	float Multiplier = DefaultSellMultiplier;

	// 테이블에서 이 아이템 클래스의 시세 배율 검색
	if (ShopItemTable)
	{
		ShopItemTable->ForeachRow<FAbyssShopItemRow>(TEXT("SellZone"),
			[&Multiplier, Item](const FName& RowName, const FAbyssShopItemRow& Row)
			{
				if (Row.ItemClass && Item->GetClass()->IsChildOf(Row.ItemClass))
				{
					Multiplier = Row.SellPriceMultiplier;
				}
			});
	}

	return FMath::FloorToInt(Item->ItemPrice * Multiplier);
}

bool AShopSellZone::IsSellable(const AAbyssItemBase* Item) const
{
	if (!Item) return false;

	// 누군가 들고 있는 아이템은 판매 불가 (실수 방지)
	if (Item->IsPickedUp()) return false;

	// 동료 시체는 기본 판매 제외
	if (!bAllowCorpseSale && Item->IsA<AAbyssCorpseItem>()) return false;

	return true;
}

void AShopSellZone::OnRep_QuoteState()
{
	// 견적 대기 중에는 위젯 상시 표시 (전 클라)
	if (InfoWidget)
	{
		InfoWidget->SetVisibility(bQuotePending);
	}
	OnSellZoneUpdated();
}

void AShopSellZone::OnFocus_Implementation()
{
	if (InfoWidget) InfoWidget->SetVisibility(true);
}

void AShopSellZone::OnLostFocus_Implementation()
{
	// 견적 대기 중에는 시선을 떼도 계속 표시
	if (InfoWidget && !bQuotePending) InfoWidget->SetVisibility(false);
}
