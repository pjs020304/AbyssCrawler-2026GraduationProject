#include "AbyssOctopusCharacter.h"
#include "AbyssDiverCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"

AAbyssOctopusCharacter::AAbyssOctopusCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Add ability to crawl on walls/floor - typical setup for underwater creatures
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
	}
}

void AAbyssOctopusCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Create dynamic material instance for stealth
	if (GetMesh() && GetMesh()->GetMaterial(0))
	{
		BodyMaterialInstance = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
	}
	
	SetStealthMode(true); // Start in stealth
}

void AAbyssOctopusCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAbyssOctopusCharacter::SetStealthMode(bool bIsStealth)
{
	if (BodyMaterialInstance)
	{
		// 1 = stealth, 0 = visible
		BodyMaterialInstance->SetScalarParameterValue(StealthParameterName, bIsStealth ? 1.0f : 0.0f);
	}
}

void AAbyssOctopusCharacter::GrabPlayer(AAbyssDiverCharacter* Player)
{
	if (!Player || bIsGrabbing || bIsGrabCooldown) return;

	bIsGrabbing = true;
	GrabbedPlayer = Player;

	// Stop Octopus movement before attaching
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	// Disable octopus collision so it doesn't block player or world strangely while attached
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Attach to player (e.g. to the head bone)
	AttachToComponent(Player->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachBoneName);
	
	// Notify player to start DOT and input blocking
	Player->OnGrabbed(this, RequiredEscapeClicks, GrabDamagePerSecond);
}

void AAbyssOctopusCharacter::OnGrabBroken()
{
	bIsGrabbing = false;
	GrabbedPlayer = nullptr;

	// Detach and restore collision
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
	}

	// Apply a knockback or jump back logic here so the octopus backs away
	FVector BackwardDir = -GetActorForwardVector();
	UE_LOG(LogTemp, Warning, TEXT("Call On Grab Broken"));
	LaunchCharacter(BackwardDir * 1000.0f, true, true);

	// Start cooldown
	bIsGrabCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(GrabCooldownTimerHandle, this, &AAbyssOctopusCharacter::ResetGrabCooldown, GrabCooldownTime, false);
}

void AAbyssOctopusCharacter::ResetGrabCooldown()
{
	bIsGrabCooldown = false;
}
