#include "UnderwaterScooterItem.h"
#include "AbyssDiverCharacter.h"
#include "NiagaraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AUnderwaterScooterItem::AUnderwaterScooterItem()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	BubbleParticleComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BubbleParticleComp"));
	BubbleParticleComp->SetupAttachment(RootComponent);
	BubbleParticleComp->SetAutoActivate(false);
}

void AUnderwaterScooterItem::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
}

void AUnderwaterScooterItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUnderwaterScooterItem, bIsActive);
}

void AUnderwaterScooterItem::UseItem()
{
	if (OwnerCharacter && !OwnerCharacter->IsSwimming)
	{
		return;
	}

	Super::UseItem();

	if (HasAuthority())
	{
		bIsActive = true;
		OnRep_IsActive(); // Update on listen server
		
		SetActorTickEnabled(true);
		
		// Start battery drain timer
		GetWorldTimerManager().SetTimer(BatteryDrainTimer, this, &AUnderwaterScooterItem::DrainBattery, 1.0f, true);
	}
}

void AUnderwaterScooterItem::EndUseItem()
{
	Super::EndUseItem();

	if (HasAuthority())
	{
		bIsActive = false;
		OnRep_IsActive(); // Update on listen server

		SetActorTickEnabled(false);
		GetWorldTimerManager().ClearTimer(BatteryDrainTimer);
	}
}

void AUnderwaterScooterItem::OnRep_IsActive()
{
	ApplyVisualEffects();
}

void AUnderwaterScooterItem::ApplyVisualEffects()
{
	if (bIsActive)
	{
		UE_LOG(LogTemp, Log, TEXT("Underwater Scooter Activated"));

		if (BubbleParticleComp)
		{
			BubbleParticleComp->Activate(true);
		}

		if (ScooterShakeClass && OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
			if (PC)
			{
				PC->ClientStartCameraShake(ScooterShakeClass);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Underwater Scooter Deactivated"));

		if (BubbleParticleComp)
		{
			BubbleParticleComp->Deactivate();
		}

		if (ScooterShakeClass && OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
			if (PC)
			{
				// Stop the shake, false for gradual stop
				PC->ClientStopCameraShake(ScooterShakeClass, false);
			}
		}
	}
}

void AUnderwaterScooterItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bIsActive && OwnerCharacter)
	{
		if (!OwnerCharacter->IsSwimming)
		{
			EndUseItem();
			return;
		}

		if (OwnerCharacter->FirstPersonCameraComponent)
		{
			FVector ForwardDir = OwnerCharacter->FirstPersonCameraComponent->GetForwardVector();
			OwnerCharacter->GetCharacterMovement()->AddForce(ForwardDir * PropulsionForce);
		}
	}
}

void AUnderwaterScooterItem::DrainBattery()
{
	if (!HasAuthority() || !OwnerCharacter || !BatteryConsumeEffectClass) return;

	UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(OwnerCharacter, OwnerCharacter);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BatteryConsumeEffectClass, 1.0f, Context);

		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}
