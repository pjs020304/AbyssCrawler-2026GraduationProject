#include "HarpoonGunItem.h"
#include "HarpoonProjectile.h"
#include "AbyssDiverCharacter.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"


void AHarpoonGunItem::UseItem()
{
	Super::UseItem();

	if (OwnerCharacter && ProjectileClass)
	{
		// 카메라가 바라보는 방향으로 발사 위치와 회전값 계산
		FVector SpawnLocation = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorForwardVector() * 100.0f);
		FRotator SpawnRotation = OwnerCharacter->GetControlRotation(); // 플레이어 시선 방향

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerCharacter;
		SpawnParams.Instigator = OwnerCharacter; // 총을 쏜 주체를 플레이어로 명시

		// 월드에 발사체 소환
		GetWorld()->SpawnActor<AHarpoonProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	}
}