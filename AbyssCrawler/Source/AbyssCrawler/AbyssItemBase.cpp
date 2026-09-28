#include "AbyssItemBase.h"
#include "AbyssDiverCharacter.h" // 캐릭터 함수 호출용
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "AbyssSubmarine.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PhysicsReplicationInterface.h"

AAbyssItemBase::AAbyssItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	// 1. 충돌 박스를 루트로 설정 (물리 시뮬레이션/이동 리플리케이션은 루트 프리미티브 기준으로 동작)
	CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;
	CollisionComp->SetBoxExtent(FVector(20.f));

	// 2. 레이캐스트(LineTrace)에 맞을 수 있도록 충돌 설정 활성화
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 3. 메시는 시각 전용 자식 컴포넌트 — BP에서 회전/이동 자유롭게 조절 가능
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(RootComponent);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetCollisionProfileName(TEXT("NoCollision"));

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

void AAbyssItemBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (ItemMesh)
	{
		// 구조 변경 전(메시=루트) 만들어진 BP들은 ItemMesh에 Simulate Physics가 켜져 있을 수 있다.
		// 메시는 시각 전용 자식이 됐으므로 물리가 켜져 있으면 액터에서 분리되어 떨어진다 —
		// BP에 남아 있는 물리 설정을 루트 박스로 이관하고 메시는 항상 물리/충돌 없음으로 강제한다.
		const bool bMeshWantsPhysics = ItemMesh->BodyInstance.bSimulatePhysics;

		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemMesh->SetCollisionProfileName(TEXT("NoCollision"));

		if (bMeshWantsPhysics && CollisionComp && !bPickedUp)
		{
			CollisionComp->SetSimulatePhysics(true);
		}
	}
}

void AAbyssItemBase::UseItem()
{
	UE_LOG(LogTemp, Log, TEXT("Item Used: %s"), *ItemName);

	// 1. 서버에서만 소모 로직을 처리합니다.
	if (!HasAuthority() || !OwnerCharacter) return;

	// 배터리를 쓰지 않는 아이템은 소모 단계를 건너뛰고 바로 효과음만 재생한다.
	if (!BatteryConsumeEffectClass)
	{
		PlayItemSound(UseSound);
		return;
	}

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

			// 배터리 차감까지 통과한, 실제로 성사된 사용에만 효과음을 붙인다.
			PlayItemSound(UseSound);

			// 4. (옵션) 이 아래에 실제 아이템 고유의 기능(빛 켜기 등)을 구현하거나 서브클래스에서 Super::UseItem() 호출 후 구현합니다.
		}
	}
}

void AAbyssItemBase::EndUseItem()
{
	UE_LOG(LogTemp, Log, TEXT("Item Use Ended: %s"), *ItemName);
}

void AAbyssItemBase::NotifyUnequipped()
{
	// 기본 동작: 지속 소모 정지 (서브클래스에서 필요 시 super 호출 후 추가 처리)
	StopPassiveDrain();
}

void AAbyssItemBase::PlayItemSound(USoundBase* Sound)
{
	// 소리는 서버가 판정한 "실제로 일어난 사건"에만 붙인다.
	// 클라이언트가 각자 재생하면 서버가 거부한 사용에도 소리가 나기 때문이다.
	if (!HasAuthority() || !Sound) return;

	Multicast_PlayItemSound(Sound);
}

void AAbyssItemBase::Multicast_PlayItemSound_Implementation(USoundBase* Sound)
{
	if (!Sound) return;

	UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
}

void AAbyssItemBase::Multicast_PlayPickupSound_Implementation()
{
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			PickupSound,
			GetActorLocation()
		);
	}
}

void AAbyssItemBase::StartPassiveDrain()
{
	if (!HasAuthority()) return;
	if (DrainRatePerSecond <= 0.0f) return;

	// 이미 실행 중이면 중복 시작 방지
	if (GetWorldTimerManager().IsTimerActive(PassiveDrainTimer)) return;

	GetWorldTimerManager().SetTimer(
		PassiveDrainTimer,
		this,
		&AAbyssItemBase::OnPassiveDrainTick,
		1.0f,  // 1초마다
		true   // 반복
	);

	UE_LOG(LogTemp, Log, TEXT("[PassiveDrain] Started for %s (%.1f/sec)"), *ItemName, DrainRatePerSecond);
}

void AAbyssItemBase::StopPassiveDrain()
{
	if (!HasAuthority()) return;

	if (GetWorldTimerManager().IsTimerActive(PassiveDrainTimer))
	{
		GetWorldTimerManager().ClearTimer(PassiveDrainTimer);
		UE_LOG(LogTemp, Log, TEXT("[PassiveDrain] Stopped for %s"), *ItemName);
	}
}

void AAbyssItemBase::OnPassiveDrainTick()
{
	if (!HasAuthority() || !OwnerCharacter || !BatteryConsumeEffectClass) return;

	UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
	if (!ASC) return;

	// 배터리 잔량 확인
	float CurrentBattery = ASC->GetNumericAttribute(UAbyssAttributeSet::GetBatteryAttribute());

	// 잔량이 소모량보다 적으면 고갈 처리
	if (CurrentBattery <= DrainRatePerSecond)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PassiveDrain] Battery depleted for %s"), *ItemName);
		StopPassiveDrain();
		OnBatteryDepleted();
		return;
	}

	// SetByCaller 태그로 DrainRatePerSecond 만큼 차감
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddInstigator(this, OwnerCharacter);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BatteryConsumeEffectClass, 1.0f, Context);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(BatteryCostTag, -DrainRatePerSecond);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		UE_LOG(LogTemp, Log, TEXT("[PassiveDrain] %s drained %.1f battery"), *ItemName, DrainRatePerSecond);
	}
}

void AAbyssItemBase::OnBatteryDepleted()
{
	// 기본 구현: 아무것도 하지 않음 (서브클래스에서 override하여 처리)
	UE_LOG(LogTemp, Warning, TEXT("[PassiveDrain] OnBatteryDepleted (base) for %s"), *ItemName);
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

			UE_LOG(LogTemp, Warning, TEXT("[PickupSound] Item=%s PickupSound=%s Diver=%s"),
				*GetName(),
				PickupSound ? *PickupSound->GetName() : TEXT("NULL"),
				*Diver->GetName());

			if (PickupSound)
			{
				Diver->Client_PlaySound2D(PickupSound);
			}
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

void AAbyssItemBase::SetAsPickedUp(AAbyssDiverCharacter* NewOwnerCharacter, USceneComponent* AttachParent, bool bVisibleInHand, FName AttachSocketName)
{
	OwnerCharacter = NewOwnerCharacter;
	bPickedUp = true;
	bStowed = false; // 잠수함 이동 중 주워도 고정 상태는 해제

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
		AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
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

	// 이동 중인 잠수함 안에 떨어뜨렸다면 바로 선체에 고정 (안 그러면 고속으로 움직이는 바닥을 뚫고 빠진다)
	if (HasAuthority())
	{
		for (TActorIterator<AAbyssSubmarine> It(GetWorld()); It; ++It)
		{
			if (It->TryStowItem(this))
			{
				break;
			}
		}
	}
}

void AAbyssItemBase::SetStowedIn(USceneComponent* Parent)
{
	if (!HasAuthority() || bPickedUp) return;

	UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent());

	if (Parent)
	{
		bWasSimulatingBeforeStow = RootPrim && RootPrim->IsSimulatingPhysics();
		bStowed = true;

		// 물리를 먼저 꺼야 부착이 유지된다 (엔진은 시뮬레이션 중인 컴포넌트의 부착을 즉시 풀어버린다)
		if (RootPrim)
		{
			RootPrim->SetSimulatePhysics(false);
		}
		AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		bStowed = false;
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		if (RootPrim && bWasSimulatingBeforeStow)
		{
			RootPrim->SetSimulatePhysics(true);

			// 고정 중에는 키네마틱으로 잠수함과 함께 (초속 수십 m로) 움직였기 때문에
			// 물리를 다시 켜면 그 속도가 그대로 남는다. 그대로 두면 얇은 바닥을 뚫고 빠져나가므로 반드시 0으로.
			RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			RootPrim->WakeAllRigidBodies();
		}
		bWasSimulatingBeforeStow = false;
	}

	ForceNetUpdate();
}

void AAbyssItemBase::StopLocalPhysicsForStow()
{
	UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent());
	if (!RootPrim) return;

	RootPrim->SetSimulatePhysics(false);

	// 고정 직전의 물리 복제 목표(수면 위치)가 남아 있으면 계속 그쪽으로 끌려갈 수 있으므로 제거
	if (UWorld* World = GetWorld())
	{
		if (FPhysScene* PhysScene = World->GetPhysicsScene())
		{
			if (IPhysicsReplication* PhysicsReplication = PhysScene->GetPhysicsReplication())
			{
				PhysicsReplication->RemoveReplicatedTarget(RootPrim);
			}
		}
	}
}

void AAbyssItemBase::OnRep_AttachmentReplication()
{
	// 클라이언트에서 부착 정보가 bStowed보다 먼저 도착하면 루트가 아직 물리 시뮬레이션 중이라
	// 엔진이 부착을 즉시 풀어버린다(아이템이 수면에 남고 잠수함만 내려가 "안 보이는" 원인).
	// 고정 상태라면 물리를 먼저 끄고 부착을 적용한다.
	if (bStowed)
	{
		StopLocalPhysicsForStow();
	}

	Super::OnRep_AttachmentReplication();
}

void AAbyssItemBase::OnRep_Stowed()
{
	if (bStowed)
	{
		// 부착 정보가 먼저 와서 (물리 때문에) 부착이 풀렸던 경우를 대비해,
		// 물리를 끈 뒤 이미 받아 둔 부착 정보를 다시 적용한다.
		StopLocalPhysicsForStow();
		if (GetAttachmentReplication().AttachParent)
		{
			Super::OnRep_AttachmentReplication();
		}
	}
	// 해제 시에는 부착 해제 복제 → OnRep_ReplicatedMovement(bRepPhysics)가 물리 상태를 다시 맞춰 준다.
}

void AAbyssItemBase::OnRep_PickedUp()
{
	ApplyPickedUpState();

}

void AAbyssItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssItemBase, bPickedUp);
	DOREPLIFETIME(AAbyssItemBase, bStowed);
}

void AAbyssItemBase::ApplyPickedUpState()
{
	if (bPickedUp)
	{
		SetActorEnableCollision(false);

		if (CollisionComp)
		{
			CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			CollisionComp->SetCollisionProfileName(TEXT("NoCollision"));
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

		if (CollisionComp)
		{
			CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}
	}
}

