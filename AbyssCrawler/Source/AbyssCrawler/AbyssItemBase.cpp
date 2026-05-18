#include "AbyssItemBase.h"
#include "AbyssDiverCharacter.h" // 캐릭터 함수 호출용
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"

AAbyssItemBase::AAbyssItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

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

	BatteryCost = 10.0f;
}

void AAbyssItemBase::UseItem()
{
	UE_LOG(LogTemp, Log, TEXT("Item Used: %s"), *ItemName);

	// 1. 서버에서만 소모 로직을 처리합니다.
	if (!HasAuthority() || !OwnerCharacter || !BatteryConsumeEffectClass) return;

	UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
	if (ASC)
	{
		// 2. [검사] 현재 배터리 잔량이 소모량보다 많은지 확인
		float CurrentBattery = ASC->GetNumericAttribute(UAbyssAttributeSet::GetBatteryAttribute());
		if (CurrentBattery < BatteryCost)
		{
			UE_LOG(LogTemp, Warning, TEXT("Low Battery: %s"), *ItemName);
			return; // 배터리 부족 시 실행 취소
		}

		// 3. [소모] SetByCaller를 통해 다이내믹하게 배터리 차감 적용
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(this, OwnerCharacter);

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BatteryConsumeEffectClass, 1.0f, Context);
		if (SpecHandle.IsValid())
		{
			// 블루프린트 GE에 우리가 정한 소모량(-BatteryCost)을 태그를 통해 주입합니다.
			SpecHandle.Data->SetSetByCallerMagnitude(BatteryCostTag, -BatteryCost);
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			UE_LOG(LogTemp, Log, TEXT("Item Used: %s, Battery Consumed: %f"), *ItemName, BatteryCost);

			// 4. (옵션) 이 아래에 실제 아이템 고유의 기능(빛 켜기 등)을 구현하거나 서브클래스에서 Super::UseItem() 호출 후 구현합니다.
		}
	}
}

void AAbyssItemBase::EndUseItem()
{
	UE_LOG(LogTemp, Log, TEXT("Item Use Ended: %s"), *ItemName);
}

// [핵심] E키를 눌러 상호작용했을 때 실행되는 함수
void AAbyssItemBase::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bPickedUp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Item] Already picked up: %s"), *GetName());
		return;
	}

	// 1. 상호작용한 사람이 플레이어 캐릭터인지 확인
	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
	if (Diver)
	{
		// 2. 캐릭터의 인벤토리에 넣기 시도
		if (Diver->AddItemToInventory(this))
		{
			// 3. 인벤토리에 성공적으로 들어갔다면, 바닥에서 안 보이게 처리
			//ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 더 이상 상호작용 안 되게 충돌 끄기
			//ItemMesh->SetVisibility(false); // 눈에 안 보이게 숨기기

			//UE_LOG(LogTemp, Log, TEXT("%s Get! 가격: %d"), *ItemName, ItemPrice);

			//OwnerCharacter = Diver;

		}
		else
		{
			// 인벤토리가 꽉 찼을 때의 처리 (UI 메시지 출력 등)
			UE_LOG(LogTemp, Warning, TEXT("[Abyss] Getting Fail"));
		}
	}
	else
	{
		return;
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

void AAbyssItemBase::SetAsPickedUp(AAbyssDiverCharacter* NewOwnerCharacter, USceneComponent* AttachParent, bool bVisibleInHand)
{
	OwnerCharacter = NewOwnerCharacter;
	bPickedUp = true;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrim->SetSimulatePhysics(false);
		RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RootPrim->SetCollisionProfileName(TEXT("NoCollision"));
	}

	SetActorEnableCollision(false);

	if (AttachParent)
	{
		AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	SetActorHiddenInGame(!bVisibleInHand);

	if (InteractWidgetComp)
	{
		InteractWidgetComp->SetVisibility(false);
	}
}

void AAbyssItemBase::SetAsDropped(const FVector& DropLocation, const FRotator& DropRotation, const FVector& ThrowImpulse)
{
	OwnerCharacter = nullptr;
	bPickedUp = false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	SetActorLocation(DropLocation);
	SetActorRotation(DropRotation);

	ApplyPickedUpState();

	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrim->SetSimulatePhysics(true);
		RootPrim->WakeAllRigidBodies();
		RootPrim->AddImpulse(ThrowImpulse, NAME_None, true);
	}
}

void AAbyssItemBase::OnRep_PickedUp()
{
	ApplyPickedUpState();

}

void AAbyssItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssItemBase, bPickedUp);
}

void AAbyssItemBase::ApplyPickedUpState()
{
	if (bPickedUp)
	{
		SetActorEnableCollision(false);

		if (ItemMesh)
		{
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetCollisionProfileName(TEXT("NoCollision"));
		}

		if (InteractWidgetComp)
		{
			InteractWidgetComp->SetVisibility(false);
		}
	}
	else
	{
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);

		if (ItemMesh)
		{
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}
	}
}

