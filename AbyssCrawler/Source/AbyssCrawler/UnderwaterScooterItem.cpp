// Fill out your copyright notice in the Description page of Project Settings.

#include "UnderwaterScooterItem.h"
#include "AbyssDiverCharacter.h"
#include "NiagaraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

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
	// 수중에서만 작동
	if (OwnerCharacter && !OwnerCharacter->IsSwimming)
	{
		return;
	}

	// 주의: Super::UseItem()의 일회성 배터리 소모는 호출하지 않음.
	// 수중 추진기는 지속 소모(PassiveDrain)만 사용.

	if (HasAuthority())
	{
		bIsActive = true;
		OnRep_IsActive(); // 리슨 서버에서 즉시 적용

		SetActorTickEnabled(true);

		// 베이스 클래스의 지속 배터리 소모 시작
		// DrainRatePerSecond는 BP_UnderwaterScooter 에디터에서 설정
		StartPassiveDrain();
	}
}

void AUnderwaterScooterItem::EndUseItem()
{
	Super::EndUseItem();

	if (HasAuthority())
	{
		bIsActive = false;
		OnRep_IsActive(); // 리슨 서버에서 즉시 적용

		SetActorTickEnabled(false);

		// 베이스 클래스의 지속 배터리 소모 정지
		StopPassiveDrain();
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

void AUnderwaterScooterItem::OnBatteryDepleted()
{
	UE_LOG(LogTemp, Warning, TEXT("[Scooter] Battery depleted! Stopping scooter."));

	// 배터리 고갈 시 추진기 자동 정지
	EndUseItem();
}

void AUnderwaterScooterItem::NotifyUnequipped()
{
	// 슬롯에서 해제되면 추진기 정지 + 지속 소모 정지
	if (bIsActive)
	{
		EndUseItem();
	}

	// 부모의 NotifyUnequipped 호출 (StopPassiveDrain 포함)
	Super::NotifyUnequipped();
}
