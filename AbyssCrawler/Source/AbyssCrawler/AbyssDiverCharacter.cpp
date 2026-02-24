#include "AbyssDiverCharacter.h"
#include "AbyssCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// 중요: 커스텀 무브먼트 컴포넌트를 사용하려면 FObjectInitializer를 받는 생성자를 써야 함
AAbyssDiverCharacter::AAbyssDiverCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAbyssCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 캡슐 컴포넌트 설정 (기획서 신장 1.2m 고려 필요, 기본은 1.8m 정도임)
	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.0f);

	// 2. 카메라 설정 (1인칭)
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), TEXT("head"));

	// 카메라 위치 미세 조정 (눈 위치로 맞춤, 모델에 따라 다름)
	FirstPersonCameraComponent->SetRelativeLocation(FVector(100.f, 0.f, 0.f));

	// 컨트롤러 회전에 따라 카메라가 돌도록 설정
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// 3. 메쉬 설정 (기본 메쉬)
	GetMesh()->SetOwnerNoSee(false); 
	GetMesh()->bCastHiddenShadow = false;
	// 나중에 팔만 보이는 1인칭 전용 메쉬를 추가하는 것이 좋음

	// 4. 회전 설정
	// 컨트롤러가 회전할 때 캐릭터 몸통도 같이 돌지 여부
	// FPS이므로 Yaw는 같이 도는 게 일반적
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;



}

void AAbyssDiverCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enhanced Input 매핑
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 1. 인벤토리 초기화 (빈 슬롯 5개 생성)
	Inventory.Init(nullptr, 5);
	CurrentSlotIndex = 0;

	// 2. 기본 아이템(손전등) 생성 및 지급
	if (DefaultItemClass)
	{
		// 월드에 아이템 액터 생성
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		AAbyssItemBase* NewItem = GetWorld()->SpawnActor<AAbyssItemBase>(DefaultItemClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (NewItem)
		{
			// 인벤토리 0번 슬롯에 넣기
			Inventory[0] = NewItem;
			Inventory[1] = NewItem;
			Inventory[2] = NewItem;
			Inventory[3] = NewItem;
			Inventory[4] = NewItem;

			// [중요] 아이템을 카메라에 붙이기 (1인칭 시점)
			NewItem->AttachToComponent(FirstPersonCameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		}
	}
	
	OnInventoryUpdated();

}

void AAbyssDiverCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 여기에 나중에 산소 소모 등의 로직 추가
}

void AAbyssDiverCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAbyssDiverCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAbyssDiverCharacter::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AAbyssDiverCharacter::StartAscend);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAbyssDiverCharacter::StopAscend);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AAbyssDiverCharacter::StartDescend);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AAbyssDiverCharacter::StopDescend);

		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::StartDash);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Completed, this, &AAbyssDiverCharacter::StopDash);

		// 아이템 사용 (마우스 좌클릭)
		EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::UseCurrentItem);

		// 슬롯 변경 (숫자키 1, 2)
		EnhancedInputComponent->BindAction(Slot1Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot1);
		EnhancedInputComponent->BindAction(Slot2Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot2);
		EnhancedInputComponent->BindAction(Slot3Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot3);
		EnhancedInputComponent->BindAction(Slot4Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot4);
		EnhancedInputComponent->BindAction(Slot5Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot5);

		// 상호작용 Interaction
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::TryInteract);


	}
}

void AAbyssDiverCharacter::StartDash()
{
	// 커스텀 무브먼트 컴포넌트 가져오기
	if (UAbyssCharacterMovementComponent* MyCMC = Cast<UAbyssCharacterMovementComponent>(GetCharacterMovement()))
	{
		MyCMC->SetSprinting(true);
	}
}

void AAbyssDiverCharacter::StopDash()
{
	if (UAbyssCharacterMovementComponent* MyCMC = Cast<UAbyssCharacterMovementComponent>(GetCharacterMovement()))
	{
		MyCMC->SetSprinting(false);
	}
}

void AAbyssDiverCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// [핵심] 수영 모드일 때는 카메라가 보는 방향(Z축 포함)으로 이동해야 함 (6DOF)
		bool bIsSwimming = GetCharacterMovement()->IsSwimming();

		if (bIsSwimming)
		{
			// 수영 중: ControlRotation을 사용하여 3차원 방향 획득
			FRotator ControlRot = GetControlRotation();
			FVector ForwardDir = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
			FVector RightDir = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);

			AddMovementInput(ForwardDir, MovementVector.Y);
			AddMovementInput(RightDir, MovementVector.X);

			//UE_LOG(LogTemp, Warning, TEXT("Swiming now"))

		}
		else
		{
			// 걷기 중: 바닥에 붙어 다녀야 하므로 Yaw 회전만 고려 (Z축 배제)
			FRotator YawRot(0, GetControlRotation().Yaw, 0);
			FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
			FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

			AddMovementInput(ForwardDir, MovementVector.Y);
			AddMovementInput(RightDir, MovementVector.X);
		}
	}
}

void AAbyssDiverCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AAbyssDiverCharacter::StartAscend()
{
	if (GetCharacterMovement()->IsSwimming())
	{
		// 수영 중 Space: 수직 상승
		AddMovementInput(FVector::UpVector, 1.0f);
		//UE_LOG(LogTemp, Warning, TEXT("Swiming Ascend now"))
	}
	else
	{
		// 걷기 중 Space: 점프
		Jump();
	}
}

void AAbyssDiverCharacter::StopAscend()
{
	StopJumping();
}

void AAbyssDiverCharacter::StartDescend()
{
	if (GetCharacterMovement()->IsSwimming())
	{
		// 수영 중 Ctrl: 수직 하강
		AddMovementInput(FVector::UpVector, -1.0f);
	}
	else
	{
		// 걷기 중 Ctrl: 웅크리기 (추후 구현)
		Crouch();
	}
}

void AAbyssDiverCharacter::StopDescend()
{
	UnCrouch();
}

void AAbyssDiverCharacter::UseCurrentItem()
{
	// 현재 슬롯에 아이템이 있는지 확인
	if (Inventory.IsValidIndex(CurrentSlotIndex) && Inventory[CurrentSlotIndex] != nullptr)
	{
		Inventory[CurrentSlotIndex]->UseItem();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("현재 슬롯이 비어있습니다."));
	}
}

void AAbyssDiverCharacter::SwitchToSlot(int32 NewIndex)
{
	if (CurrentSlotIndex == NewIndex) return;

	// 기존 아이템 숨기기 등의 로직이 필요하다면 여기에 작성
	// 예: Inventory[CurrentSlotIndex]->SetActorHiddenInGame(true);

	CurrentSlotIndex = NewIndex;
	UE_LOG(LogTemp, Log, TEXT("슬롯 변경: %d"), CurrentSlotIndex + 1);

	// 새 아이템 보이기
	// 예: Inventory[CurrentSlotIndex]->SetActorHiddenInGame(false);

	OnInventoryUpdated();
}

void AAbyssDiverCharacter::EquipSlot1() { SwitchToSlot(0); }
void AAbyssDiverCharacter::EquipSlot2() { SwitchToSlot(1); }
void AAbyssDiverCharacter::EquipSlot3() { SwitchToSlot(2); }
void AAbyssDiverCharacter::EquipSlot4() { SwitchToSlot(3); }
void AAbyssDiverCharacter::EquipSlot5() { SwitchToSlot(4); }


// E키를 눌렀을 때 실행되는 함수
void AAbyssDiverCharacter::TryInteract()
{
	if (!FirstPersonCameraComponent) return;

	// 1. 레이캐스트 시작점(카메라 위치)과 끝점(카메라가 바라보는 방향 * 거리) 계산
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * InteractDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // 내 캐릭터는 검사에서 제외

	// 2. 눈에 보이는 물체(ECC_Visibility: 기본으로 StaticMesh Block, Ignore부분 설정 가능)를 기준으로 레이저 쏘기
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);

	// (디버그용)
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);

	// 3. 라인트레이스에 걸렸을 때
	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();

		// 4. 그 물체가 상호작용 인터페이스를 상속받았는지 확인
		if (HitActor->Implements<UAbyssInteractionInterface>())
		{
			// 5. 인터페이스의 Interact 함수 실행 
			IAbyssInteractionInterface::Execute_Interact(HitActor, this);
		}
	}
}