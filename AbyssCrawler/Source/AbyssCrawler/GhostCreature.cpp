#include "GhostCreature.h"

#include "AbyssDiverCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

AGhostCreature::AGhostCreature()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	// 무적 + 비물질: 모든 충돌을 끈다(공격/벽 통과). 피격 콜리전이 없어 하푼 등에 맞지 않음.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetCollisionProfileName(TEXT("NoCollision"));
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	}

	// 중력/물리 이동을 쓰지 않는다(매 틱 직접 SetActorLocation으로 이동).
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->GravityScale = 0.f;
		Move->SetMovementMode(MOVE_Flying);
		Move->bUseControllerDesiredRotation = false;
		Move->bOrientRotationToMovement = false;
	}

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
}

void AGhostCreature::BeginPlay()
{
	Super::BeginPlay();
}

void AGhostCreature::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 이동/타깃팅은 서버 권위. 클라이언트는 복제된 위치를 보간만 한다.
	if (!HasAuthority())
	{
		return;
	}

	// 주기적으로 가장 가까운 다이버 재선정.
	RetargetAccum += DeltaTime;
	if (RetargetAccum >= RetargetInterval || !CurrentTarget.IsValid())
	{
		RetargetAccum = 0.f;
		CurrentTarget = FindNearestDiver();
	}

	AAbyssDiverCharacter* Target = CurrentTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector MyLoc = GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector ToTarget = TargetLoc - MyLoc;
	const float Dist = ToTarget.Size();
	if (Dist <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Dir = ToTarget / Dist;

	// 벽을 무시하고 직선으로 천천히 이동(스윕 없이 통과).
	const FVector NewLoc = MyLoc + Dir * MoveSpeed * DeltaTime;
	SetActorLocation(NewLoc, /*bSweep=*/false);

	// 진행 방향으로 부드럽게 회전.
	const FRotator TargetRot = Dir.Rotation();
	const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnSpeed);
	SetActorRotation(NewRot);
}

AAbyssDiverCharacter* AGhostCreature::FindNearestDiver() const
{
	AAbyssDiverCharacter* Nearest = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	const FVector MyLoc = GetActorLocation();
	for (TActorIterator<AAbyssDiverCharacter> It(GetWorld()); It; ++It)
	{
		AAbyssDiverCharacter* Diver = *It;
		if (!IsValid(Diver) || Diver->bIsDead)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLoc, Diver->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = Diver;
		}
	}

	return Nearest;
}
