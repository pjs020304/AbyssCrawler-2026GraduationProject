#include "AbyssRechargeStation.h"
#include "Components/WidgetComponent.h"
#include "AbyssDiverCharacter.h"
#include "AbilitySystemComponent.h"

AAbyssRechargeStation::AAbyssRechargeStation()
{
	bReplicates = true;
	bIsOnCooldown = false;
	RechargeCooldown = 5.0f; // 5초 쿨타임

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionProfileName(TEXT("BlockAllDynamic")); // 레이저에 맞아야 함
	RootComponent = MeshComp;

	InteractWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractWidget"));
	InteractWidget->SetupAttachment(RootComponent);
	InteractWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractWidget->SetVisibility(false);
}

void AAbyssRechargeStation::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsOnCooldown) return;

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(InstigatorActor);
	if (Diver && RechargeEffectClass)
	{
		UAbilitySystemComponent* ASC = Diver->GetAbilitySystemComponent();
		if (ASC)
		{
			// 플레이어에게 충전 이펙트(GE) 적용!
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddInstigator(this, this);
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(RechargeEffectClass, 1.0f, Context);

			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				UE_LOG(LogTemp, Warning, TEXT("Attribute Set Recharge Complete!"));

				// 쿨타임 진입 (무한 딸깍 방지)
				bIsOnCooldown = true;
				GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &AAbyssRechargeStation::ResetCooldown, RechargeCooldown, false);
			}
		}
	}
}

void AAbyssRechargeStation::ResetCooldown()
{
	bIsOnCooldown = false;
}

void AAbyssRechargeStation::OnFocus_Implementation()
{
	if (InteractWidget) InteractWidget->SetVisibility(true);
}

void AAbyssRechargeStation::OnLostFocus_Implementation()
{
	if (InteractWidget) InteractWidget->SetVisibility(false);
}