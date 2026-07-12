#include "ShopItemDisplay.h"
#include "AbyssShopTypes.h"
#include "Components/WidgetComponent.h"
#include "AbyssGameState.h"
#include "AbyssDiverCharacter.h"
#include "AbyssItemBase.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AShopItemDisplay::AShopItemDisplay()
{
	bReplicates = true;
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = MeshComp;

	PriceWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PriceWidget"));
	PriceWidget->SetupAttachment(RootComponent);
	PriceWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PriceWidget->SetDrawSize(FVector2D(250.0f, 100.0f));

	PriceWidget->SetVisibility(false);
}

void AShopItemDisplay::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 진열 상태 전체를 모든 클라이언트에 동기화
	DOREPLIFETIME(AShopItemDisplay, SelectedItemClass);
	DOREPLIFETIME(AShopItemDisplay, SelectedItemPrice);
	DOREPLIFETIME(AShopItemDisplay, SelectedItemName);
	DOREPLIFETIME(AShopItemDisplay, bIsRestocking);
	DOREPLIFETIME(AShopItemDisplay, bSoldOut);
}

void AShopItemDisplay::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SelectRandomItem();
	}
}

// [서버] 테이블(우선) 또는 레거시 ItemPool에서 가중치 랜덤으로 상품 선택
void AShopItemDisplay::SelectRandomItem()
{
	// 후보 목록 구성: (클래스, 가중치, 가격 재정의)
	struct FCandidate
	{
		TSubclassOf<AAbyssItemBase> ItemClass;
		float Weight = 1.0f;
		int32 PriceOverride = 0;
	};
	TArray<FCandidate> Candidates;

	if (ShopItemTable)
	{
		ShopItemTable->ForeachRow<FAbyssShopItemRow>(TEXT("ShopItemDisplay"),
			[&Candidates](const FName& RowName, const FAbyssShopItemRow& Row)
			{
				if (Row.ItemClass && Row.Weight > 0.0f)
				{
					Candidates.Add({ Row.ItemClass, Row.Weight, Row.PriceOverride });
				}
			});
	}
	else
	{
		for (const TSubclassOf<AAbyssItemBase>& PoolClass : ItemPool)
		{
			if (PoolClass)
			{
				Candidates.Add({ PoolClass, 1.0f, 0 });
			}
		}
	}

	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] %s: 상품 후보가 없습니다 (테이블/풀 비어있음)"), *GetName());
		return;
	}

	// 가중치 합 비례 선택
	float TotalWeight = 0.0f;
	for (const FCandidate& C : Candidates) { TotalWeight += C.Weight; }

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	const FCandidate* Picked = &Candidates.Last();
	for (const FCandidate& C : Candidates)
	{
		Roll -= C.Weight;
		if (Roll <= 0.0f) { Picked = &C; break; }
	}

	SelectedItemClass = Picked->ItemClass;

	if (const AAbyssItemBase* DefaultItem = SelectedItemClass.GetDefaultObject())
	{
		SelectedItemPrice = (Picked->PriceOverride > 0) ? Picked->PriceOverride : DefaultItem->ItemPrice;
		SelectedItemName = DefaultItem->ItemName;
	}

	UpdateVisuals();
}

void AShopItemDisplay::OnRep_ShopState()
{
	// 서버로부터 진열 상태를 전달받으면 비주얼 갱신 (클라이언트)
	UpdateVisuals();
}

void AShopItemDisplay::UpdateVisuals()
{
	const bool bAvailable = !bIsRestocking && !bSoldOut && SelectedItemClass != nullptr;

	if (bAvailable)
	{
		if (const AAbyssItemBase* DefaultItem = SelectedItemClass.GetDefaultObject())
		{
			if (DefaultItem->ItemMesh)
			{
				MeshComp->SetStaticMesh(DefaultItem->ItemMesh->GetStaticMesh());
			}
		}
	}
	MeshComp->SetVisibility(bAvailable);

	// 가격표 텍스트/재입고 연출 등은 블루프린트에서 처리
	OnDisplayUpdated();
}

void AShopItemDisplay::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;

	// 재입고 중이거나 매진이면 구매 불가
	if (bIsRestocking || bSoldOut || !SelectedItemClass) return;

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();
	if (!Diver || !GS) return;

	if (!Diver->HasEmptyInventorySlot())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] 인벤토리가 가득 차 구매 불가"));
		return;
	}

	// 팀 공유 재화 차감 (잔액 부족 시 false)
	if (!GS->ConsumeSharedMoney(SelectedItemPrice))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] 잔액 부족: %d 필요"), SelectedItemPrice);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAbyssItemBase* NewItem = GetWorld()->SpawnActor<AAbyssItemBase>(
		SelectedItemClass, GetActorLocation(), GetActorRotation(), SpawnParams);

	// 스폰 또는 인벤토리 추가 실패 시 환불 (돈만 사라지는 사고 방지)
	if (!NewItem)
	{
		GS->AddSharedMoney(SelectedItemPrice);
		UE_LOG(LogTemp, Error, TEXT("[Shop] 아이템 스폰 실패 → 환불"));
		return;
	}
	if (!Diver->AddItemToInventory(NewItem))
	{
		GS->AddSharedMoney(SelectedItemPrice);
		NewItem->Destroy();
		UE_LOG(LogTemp, Warning, TEXT("[Shop] 인벤토리 추가 실패 → 환불"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Shop] 구매 완료: %s (%d G)"), *SelectedItemName, SelectedItemPrice);

	// 판매 횟수 집계 → 매진 또는 재입고
	++PurchasedCount;
	if (MaxPurchaseCount >= 0 && PurchasedCount >= MaxPurchaseCount)
	{
		bSoldOut = true;
		UpdateVisuals();
	}
	else
	{
		BeginRestock();
	}
}

// [서버] 재입고 대기 시작
void AShopItemDisplay::BeginRestock()
{
	bIsRestocking = true;
	UpdateVisuals();

	if (RestockDelay <= 0.0f)
	{
		FinishRestock();
		return;
	}

	GetWorldTimerManager().SetTimer(RestockTimerHandle, this,
		&AShopItemDisplay::FinishRestock, RestockDelay, false);
}

// [서버] 재입고 완료: 새 상품 진열
void AShopItemDisplay::FinishRestock()
{
	if (!bKeepSameItemOnRestock)
	{
		SelectRandomItem();
	}
	bIsRestocking = false;
	UpdateVisuals();
}

void AShopItemDisplay::OnFocus_Implementation() { if (PriceWidget) PriceWidget->SetVisibility(true); }
void AShopItemDisplay::OnLostFocus_Implementation() { if (PriceWidget) PriceWidget->SetVisibility(false); }
