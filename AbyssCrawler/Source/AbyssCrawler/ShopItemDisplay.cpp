#include "ShopItemDisplay.h"
#include "Components/WidgetComponent.h"
#include "AbyssGameState.h"
#include "AbyssDiverCharacter.h" 
#include "AbyssItemBase.h"
#include "Net/UnrealNetwork.h"

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
	// 선택된 아이템 정보와 가격을 모든 클라이언트에 복제
	DOREPLIFETIME(AShopItemDisplay, SelectedItemClass);
	DOREPLIFETIME(AShopItemDisplay, SelectedItemPrice);
}

void AShopItemDisplay::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (ItemPool.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, ItemPool.Num() - 1);
			SelectedItemClass = ItemPool[RandomIndex];

			// 선택된 아이템의 기본 정보를 가져와 가격 설정
			if (SelectedItemClass)
			{
				if (AAbyssItemBase* DefaultItem = SelectedItemClass.GetDefaultObject())
				{
					SelectedItemPrice = DefaultItem->ItemPrice;
				}
			}

			// 서버에서도 비주얼을 갱신합니다.
			UpdateVisuals();
		}
	}
}

void AShopItemDisplay::OnRep_SelectedItemClass()
{
	// 서버로부터 SelectedItemClass를 전달받으면 비주얼 갱신
	UpdateVisuals();
}

void AShopItemDisplay::UpdateVisuals()
{
	if (SelectedItemClass)
	{
		// 해당 아이템의 원형을 가져와서 정보 추출
		AAbyssItemBase* DefaultItem = SelectedItemClass.GetDefaultObject();
		if (DefaultItem && DefaultItem->ItemMesh)
		{
			// 진열대 메시를 아이템의 메시로 변경
			MeshComp->SetStaticMesh(DefaultItem->ItemMesh->GetStaticMesh());

			// 가격 위젯 갱신 (블루프린트 위젯 내 함수를 호출하거나 텍스트 세팅 필요)
			SelectedItemPrice = DefaultItem->ItemPrice;
			SelectedItemName = DefaultItem->ItemName;
			 // 위젯 내 텍스트를 갱신하는 로직은 블루프린트에서 구현해야 합니다.
			 // 예: PriceWidget->GetUserWidgetObject()->SetPriceText(SelectedItemPrice);
		}
	}
}

void AShopItemDisplay::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Warning, TEXT("Interact called on ShopItemDisplay"));

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
	AAbyssGameState* GS = GetWorld()->GetGameState<AAbyssGameState>();

	if (Diver && GS && SelectedItemClass)
	{
		if (!Diver->HasEmptyInventorySlot()) return;

		if (GS->ConsumeSharedMoney(SelectedItemPrice))
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AAbyssItemBase* NewItem = GetWorld()->SpawnActor<AAbyssItemBase>(SelectedItemClass, GetActorLocation(), GetActorRotation(), SpawnParams);

			UE_LOG(LogTemp, Warning, TEXT("Spawned new item: %s"), *NewItem->GetName());


			if (NewItem)
			{
				Diver->AddItemToInventory(NewItem);
				Destroy(); // 구매 성공 시 진열대 제거
			}
		}
	}
}

void AShopItemDisplay::OnFocus_Implementation() { if (PriceWidget) PriceWidget->SetVisibility(true); }
void AShopItemDisplay::OnLostFocus_Implementation() { if (PriceWidget) PriceWidget->SetVisibility(false); }