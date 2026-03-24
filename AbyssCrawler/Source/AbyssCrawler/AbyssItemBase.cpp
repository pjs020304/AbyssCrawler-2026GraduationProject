#include "AbyssItemBase.h"
#include "AbyssDiverCharacter.h" // 캐릭터 함수 호출용

AAbyssItemBase::AAbyssItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 메쉬 생성 및 루트 설정
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;

	// 2. 레이캐스트(LineTrace)에 맞을 수 있도록 충돌 설정 활성화
	ItemMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 기본 가격 설정
	ItemPrice = 100;

	InteractWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractWidgetComp"));
	InteractWidgetComp->SetupAttachment(RootComponent);

	// 위젯 설정: Screen 공간으로 설정해야 플레이어를 항상 뚜렷하게 쳐다보며, 벽에 파묻히지 않습니다.
	InteractWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	InteractWidgetComp->SetDrawSize(FVector2D(250.0f, 100.0f));

	// 기본적으로는 안 보이게 꺼둠 (쳐다볼 때만 켜짐)
	InteractWidgetComp->SetVisibility(false);
}

void AAbyssItemBase::UseItem()
{
	UE_LOG(LogTemp, Log, TEXT("Item Used: %s"), *ItemName);
}

// [핵심] E키를 눌러 상호작용했을 때 실행되는 함수
void AAbyssItemBase::Interact_Implementation(AActor* InstigatorActor)
{
	// 1. 상호작용한 사람이 플레이어 캐릭터인지 확인
	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
	if (Diver)
	{
		// 2. 캐릭터의 인벤토리에 넣기 시도
		if (Diver->AddItemToInventory(this))
		{
			// 3. 인벤토리에 성공적으로 들어갔다면, 바닥에서 안 보이게 처리
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 더 이상 상호작용 안 되게 충돌 끄기
			//ItemMesh->SetVisibility(false); // 눈에 안 보이게 숨기기

			//UE_LOG(LogTemp, Log, TEXT("%s Get! 가격: %d"), *ItemName, ItemPrice);
		}
		else
		{
			// 인벤토리가 꽉 찼을 때의 처리 (UI 메시지 출력 등)
			UE_LOG(LogTemp, Warning, TEXT("[Abyss] Getting Fail"));
		}
	}
}

// 시선이 닿았을 때
void AAbyssItemBase::OnFocus_Implementation()
{
	if (InteractWidgetComp)
	{
		InteractWidgetComp->SetVisibility(true);
	}
}

// 시선이 벗어났을 때
void AAbyssItemBase::OnLostFocus_Implementation()
{
	if (InteractWidgetComp)
	{
		InteractWidgetComp->SetVisibility(false);
	}
}