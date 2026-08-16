#include "HarpoonGunItem.h"
#include "HarpoonProjectile.h"
#include "AbyssDiverCharacter.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"


// 서버/클라가 각자 자기 타임스탬프를 본다.
// 리슨서버 호스트는 HasAuthority가 true라 LastFireTime 하나만 쓰게 되고,
// 그 값은 UseItem()에서 발사가 성사된 뒤에야 갱신되므로 자기 자신을 막지 않는다.
float AHarpoonGunItem::GetFireCooldownRemaining() const
{
	if (FireCooldown <= 0.f || !GetWorld())
	{
		return 0.f;
	}

	const float LastUse = HasAuthority() ? LastFireTime : LastLocalFireTime;
	const float Elapsed = GetWorld()->GetTimeSeconds() - LastUse;

	return FMath::Max(0.f, FireCooldown - Elapsed);
}

bool AHarpoonGunItem::CanUseItem() const
{
	return Super::CanUseItem() && GetFireCooldownRemaining() <= 0.f;
}

void AHarpoonGunItem::NotifyUseAttempted()
{
	// 클라이언트만 기록한다. 서버에서 여기 기록해 버리면 바로 뒤에 오는 UseItem()의
	// 쿨타임 검사에 자기가 걸려서 첫 발부터 막힌다.
	if (!HasAuthority() && GetWorld())
	{
		LastLocalFireTime = GetWorld()->GetTimeSeconds();
	}
}

FVector AHarpoonGunItem::GetMuzzleLocation() const
{
	if (ItemMesh)
	{
		if (!MuzzleSocketName.IsNone() && ItemMesh->DoesSocketExist(MuzzleSocketName))
		{
			return ItemMesh->GetSocketLocation(MuzzleSocketName);
		}

		// 소켓이 없으면 메시 로컬 기준 오프셋으로 대체 (오프셋이 0이면 메시 원점)
		return ItemMesh->GetComponentTransform().TransformPosition(MuzzleOffset);
	}

	return GetActorLocation();
}

void AHarpoonGunItem::UseItem()
{
	// 쿨타임 최종 판정(서버 권위). Super보다 먼저 해야 한다 —
	// Super::UseItem()이 배터리를 차감하므로, 뒤에 두면 막힌 발사에도 배터리가 샌다.
	if (HasAuthority() && !CanUseItem())
	{
		return;
	}

	Super::UseItem();

	if (!HasAuthority())
	{
		return;
	}

	if (!OwnerCharacter || !ProjectileClass)
	{
		return;
	}

	// 1. 화면 중앙(크로스헤어)이 실제로 가리키는 지점을 먼저 구한다.
	//    총구에서 시선 방향 그대로 쏘면 총이 화면 중앙에서 벗어나 있는 만큼 탄착점이 어긋나므로,
	//    "시선 트레이스로 조준점을 찾고 → 총구에서 그 지점으로 쏘는" 표준 FPS 방식을 쓴다.
	const FVector ViewLocation = OwnerCharacter->FirstPersonCameraComponent
		? OwnerCharacter->FirstPersonCameraComponent->GetComponentLocation()
		: OwnerCharacter->GetPawnViewLocation();

	const FVector ViewDir = OwnerCharacter->GetControlRotation().Vector();
	const FVector TraceEnd = ViewLocation + ViewDir * AimTraceDistance;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(HarpoonAim), /*bTraceComplex=*/false, this);
	TraceParams.AddIgnoredActor(OwnerCharacter);

	FVector AimPoint = TraceEnd;
	FHitResult AimHit;
	if (GetWorld()->LineTraceSingleByChannel(AimHit, ViewLocation, TraceEnd, ECC_Visibility, TraceParams))
	{
		AimPoint = AimHit.ImpactPoint;
	}

	// 2. 발사는 총구에서, 방향은 총구 → 조준점
	const FVector MuzzleLocation = GetMuzzleLocation();
	FVector ShootDir = AimPoint - MuzzleLocation;

	// 조준점이 총구보다 뒤에 있으면(벽에 바짝 붙은 경우 등) 뒤로 쏘게 되므로 시선 방향으로 폴백
	if (ShootDir.IsNearlyZero() || FVector::DotProduct(ShootDir, ViewDir) <= 0.0f)
	{
		ShootDir = ViewDir;
	}

	// 여기까지 왔으면 발사가 성사된다. 쿨타임 시작.
	LastFireTime = GetWorld()->GetTimeSeconds();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter; // 총을 쏜 주체를 플레이어로 명시
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 월드에 발사체 소환
	AHarpoonProjectile* Projectile = GetWorld()->SpawnActor<AHarpoonProjectile>(
		ProjectileClass, MuzzleLocation, ShootDir.Rotation(), SpawnParams);

	// 총구가 캐릭터 몸에 붙어 있으므로, 발사 직후 자기 자신/총에 걸려 멈추지 않도록 무시 처리
	if (Projectile)
	{
		Projectile->IgnoreShooter(OwnerCharacter);
		Projectile->IgnoreShooter(this);
	}
}