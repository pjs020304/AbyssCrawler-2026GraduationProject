#include "AbyssDiverCharacter.h"
#include "AbyssCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "AbyssFlashLight.h"
#include "AbyssGameMode.h"
#include "MainHUDWidget.h"
#include "Mission/UI/MissionSelectUIWidget.h"
#include "Mission/Contents/AbyssMissionSender.h"
#include "Blueprint/UserWidget.h"
#include "Mission/Contents/AbyssMissionWorkObject.h"

// 중요: 커스텀 무브먼트 컴포넌트를 사용하려면 FObjectInitializer를 받는 생성자를 써야 함
AAbyssDiverCharacter::AAbyssDiverCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAbyssCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 캡슐 컴포넌트 설정 (기획서 신장 1.2m 고려 필요, 기본은 1.8m 정도임)
	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.0f);

	// 2. 카메라 설정 (1인칭)
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), TEXT("Bone_004"));

	// 카메라 위치 미세 조정 (눈 위치로 맞춤, 모델에 따라 다름)
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.f, 0.f));

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

	// 1. GAS 심장(컴포넌트) 생성 및 멀티플레이 설정
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// 멀티플레이 최적화 모드: 내 캐릭터(혼합 모드)로 설정하면 서버 부하를 줄이면서도 내 화면엔 즉각 반영됩니다.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 2. 자원 보관소(AttributeSet) 생성
	AttributeSet = CreateDefaultSubobject<UAbyssAttributeSet>(TEXT("AttributeSet"));
}

void AAbyssDiverCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent && AttributeSet)
	{
		// 1. [UI 바인딩]  값이 변할 때마다 OnChangedCallback 함수를 실행하라고 예약!
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetOxygenAttribute()).AddUObject(this, &AAbyssDiverCharacter::OnOxygenChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AAbyssDiverCharacter::OnHealthChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBatteryAttribute()).AddUObject(this, &AAbyssDiverCharacter::OnBatteryChangedCallback);

		// 2. [GE 적용] 산소 감소 이펙트(GE_DrainOxygen)를 내 몸에 적용하기
		if (OxygenDrainEffectClass)
		{
			// 이펙트를 적용하기 위한 포장지(Context) 만들기
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddInstigator(this, this);

			// 포장지와 이펙트를 합쳐서 적용할 준비(Spec) 완료
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(OxygenDrainEffectClass, 1.0f, Context);

			if (SpecHandle.IsValid())
			{
				// 드디어 내 몸에 산소 감소 이펙트 부착!
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// Enhanced Input 매핑
	/*if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}*/

	SetupEnhancedInput();

	if (HasAuthority())
	{
		// 1. 인벤토리 초기화 (빈 슬롯 5개 생성)
		Inventory.Init(nullptr, 5);
		CurrentSlotIndex = 0;

		// 2. 기본 아이템(손전등) 생성 및 지급
		if (DefaultItemClass)
		{
			// 월드에 아이템 액터 생성
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			AAbyssItemBase* NewItem = GetWorld()->SpawnActor<AAbyssItemBase>(
				DefaultItemClass, 
				FVector::ZeroVector, 
				FRotator::ZeroRotator, 
				SpawnParams
			);

			if (NewItem)
			{
				// 인벤토리 0번 슬롯에 넣기
				Inventory[0] = NewItem;
				/*Inventory[1] = NewItem;
				Inventory[2] = NewItem;
				Inventory[3] = NewItem;
				Inventory[4] = NewItem;

				UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(NewItem->GetRootComponent());
				if (RootPrim)
				{
					RootPrim->SetSimulatePhysics(false);
					RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}

				// 아이템을 카메라에 붙이기 (1인칭 시점)
				NewItem->AttachToComponent(FirstPersonCameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				// 애니메이션 물건을 들고 있는데, 그냥 걷는 모션 + 물건을 들고있는 모션(대부분 한손 아이템)
				// 아이템: 공격, 방해(전파), 설치(방해), 이동속도(제트팩, 손으로 들고 사용 하는거) 증가

				NewItem->SetActorEnableCollision(false);
				*/

				CurrentSlotIndex = 0;

				NewItem->SetAsPickedUp(this, FirstPersonCameraComponent, true);
				ApplyCurrentSlotVisual();

				if (IsLocallyControlled())
				{
					OnInventoryUpdated();
				}
				
			}
		}
	}
		
	//OnInventoryUpdated();

}

void AAbyssDiverCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 여기에 나중에 산소 소모 등의 로직 추가

	CheckForInteractables();
}

void AAbyssDiverCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (AbilitySystemComponent)
	{
		// 클라이언트에서도 똑같이 초기화해 주어야 UI 연동 및 예측(Prediction)이 정상 작동합니다.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

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

		EnhancedInputComponent->BindAction(ConvertAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::ConvertMove);

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

		// 버리기
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::DropItem);
	}
}

void AAbyssDiverCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssDiverCharacter, Inventory);
	DOREPLIFETIME(AAbyssDiverCharacter, CurrentSlotIndex);
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

void AAbyssDiverCharacter::Client_ShowWorkUI_Implementation()
{
	if (MainHUDRef)
	{
		MainHUDRef->BP_ShowWorkUI();
	}
}

void AAbyssDiverCharacter::Client_HideWorkUI_Implementation()
{
	if (MainHUDRef)
	{
		MainHUDRef->BP_HideWorkUI();
	}
}

void AAbyssDiverCharacter::Client_UpdateWorkProgress_Implementation(float Progress)
{
	if (MainHUDRef)
	{
		MainHUDRef->BP_UpdateWorkProgress(Progress);
	}
}

void AAbyssDiverCharacter::Client_ShowMissionComplete_Implementation(const FText& MissionName)
{
	if (MainHUDRef)
	{
		MainHUDRef->BP_ShowMissionComplete(MissionName);
	}
}

void AAbyssDiverCharacter::Client_OpenMissionSelectUI_Implementation(const TArray<FAbyssMissionData>& AvailableMissions, AAbyssMissionSender* MissionSender)
{
	if (!IsLocallyControlled()) return;
	if (!MissionSelectUIClass) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 이미 열려 있으면 새로 만들지 말고 내용만 갱신
	if (MissionSelectUIRef && MissionSelectUIRef->IsInViewport())
	{
		return;
	}

	MissionSelectUIRef = CreateWidget<UMissionSelectUIWidget>(PC, MissionSelectUIClass);
	if (!MissionSelectUIRef) return;

	MissionSelectUIRef->SetOwnerCharacter(this);
	MissionSelectUIRef->SetMissionSender(MissionSender);

	MissionSelectUIRef->AddToViewport();
	MissionSelectUIRef->InitializeMissionList(AvailableMissions);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(MissionSelectUIRef->TakeWidget());
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	bInputLockedByUI = true;

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
}

void AAbyssDiverCharacter::Server_ReleaseMissionSender_Implementation(AAbyssMissionSender* MissionSender)
{
	if (!MissionSender) return;

	MissionSender->ReleaseMissionSender(this);
}

void AAbyssDiverCharacter::ClearMissionSelectUIRef()
{
	MissionSelectUIRef = nullptr;
}

void AAbyssDiverCharacter::Client_SetWorkInputBlocked_Implementation(bool bBlocked)
{
	bIsWorkingLocked = bBlocked;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	PC->SetIgnoreMoveInput(bBlocked);
	PC->SetIgnoreLookInput(bBlocked);

	if (bBlocked)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void AAbyssDiverCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// [핵심] 수영 모드일 때는 카메라가 보는 방향(Z축 포함)으로 이동해야 함 (6DOF)
		bool bIsSwimming = GetCharacterMovement()->IsSwimming();

		if (bIsSwimming && IsSwimming)
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

void AAbyssDiverCharacter::ConvertMove()
{
	if (GetCharacterMovement()->IsSwimming()) {
		IsSwimming = !IsSwimming;
	}
}

void AAbyssDiverCharacter::UseCurrentItem()
{
	if (bInputLockedByUI) return;
	if (bIsWorkingLocked) return;

	if (!Inventory.IsValidIndex(CurrentSlotIndex) || Inventory[CurrentSlotIndex] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Abyss] Empty Slot"));
		return;
	}

	// 클라는 서버에게 사용 요청만 보냄
	if (!HasAuthority())
	{
		Server_UseCurrentItem();
		return;
	}

	// 서버만 실제 아이템 사용
	Inventory[CurrentSlotIndex]->UseItem();
}

void AAbyssDiverCharacter::SwitchToSlot(int32 NewIndex)
{
	/*
	if (CurrentSlotIndex == NewIndex) return;

	// 기존 아이템 숨기기 등의 로직이 필요하다면 여기에 작성
	// 예: Inventory[CurrentSlotIndex]->SetActorHiddenInGame(true);
	if (Inventory.IsValidIndex(CurrentSlotIndex) && Inventory[CurrentSlotIndex] != nullptr) 
	{
		Inventory[CurrentSlotIndex]->SetActorHiddenInGame(true);
	}
	CurrentSlotIndex = NewIndex;
	UE_LOG(LogTemp, Log, TEXT("Slot Change: %d"), CurrentSlotIndex + 1);

	// 새 아이템 보이기
	if (Inventory.IsValidIndex(CurrentSlotIndex) && Inventory[CurrentSlotIndex] != nullptr) {
		Inventory[CurrentSlotIndex]->SetActorHiddenInGame(false);
	}

	OnInventoryUpdated();
	*/

	UE_LOG(LogTemp, Warning, TEXT("[Slot] CurrentSlotIndex = %d, Local=%s, Authority=%s"),
		CurrentSlotIndex,
		IsLocallyControlled() ? TEXT("true") : TEXT("false"),
		HasAuthority() ? TEXT("true") : TEXT("false"));

	Server_SwitchToSlot(NewIndex);

}

void AAbyssDiverCharacter::Server_SwitchToSlot_Implementation(int32 NewIndex)
{
	if (!Inventory.IsValidIndex(NewIndex))
	{
		return;
	}

	if (CurrentSlotIndex == NewIndex)
	{
		return;
	}

	CurrentSlotIndex = NewIndex;
	ApplyCurrentSlotVisual();

	// 리슨 서버 호스트 UI 갱신 보정
	if (IsLocallyControlled())
	{
		OnInventoryUpdated();
	}
}

void AAbyssDiverCharacter::ApplyCurrentSlotVisual()
{
	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		AAbyssItemBase* Item = Inventory[i];
		if (!Item)
		{
			continue;
		}

		const bool bShouldBeVisible = (i == CurrentSlotIndex);

		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Item->GetRootComponent()))
		{
			RootPrim->SetSimulatePhysics(false);
			RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			RootPrim->SetCollisionProfileName(TEXT("NoCollision"));
		}

		Item->SetActorEnableCollision(false);

		// 현재 슬롯 아이템은 다시 카메라에 부착
		if (bShouldBeVisible && FirstPersonCameraComponent)
		{
			Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			Item->AttachToComponent(
				FirstPersonCameraComponent,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale
			);
		}

		// 장착 해제되는 손전등은 강제로 끄기
		if (!bShouldBeVisible)
		{
			if (AAbyssFlashLight* FlashLight = Cast<AAbyssFlashLight>(Item))
			{
				FlashLight->NotifyUnequipped();
			}
		}

		Item->SetActorHiddenInGame(!bShouldBeVisible);

		UE_LOG(LogTemp, Warning, TEXT("[Inventory] Slot=%d Item=%s Visible=%d"),
			i,
			*Item->GetName(),
			bShouldBeVisible ? 1 : 0);
	}
}

void AAbyssDiverCharacter::RefreshEquippedVisual()
{
	ApplyCurrentSlotVisual();
}

void AAbyssDiverCharacter::Server_OnItemCollected_Implementation()
{
	if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
	{
		GM->OnItemCollected();
	}
}

/*
void AAbyssDiverCharacter::EquipSlot1() { SwitchToSlot(0); }
void AAbyssDiverCharacter::EquipSlot2() { SwitchToSlot(1); }
void AAbyssDiverCharacter::EquipSlot3() { SwitchToSlot(2); }
void AAbyssDiverCharacter::EquipSlot4() { SwitchToSlot(3); }
void AAbyssDiverCharacter::EquipSlot5() { SwitchToSlot(4); }
*/

void AAbyssDiverCharacter::EquipSlot1() 
{ 
	if (bInputLockedByUI || bIsWorkingLocked) return;
	Server_SwitchToSlot(0); 
}
void AAbyssDiverCharacter::EquipSlot2() 
{ 
	if (bInputLockedByUI || bIsWorkingLocked) return;
	Server_SwitchToSlot(1); 
}
void AAbyssDiverCharacter::EquipSlot3() 
{ 
	if (bInputLockedByUI || bIsWorkingLocked) return;
	Server_SwitchToSlot(2); 
}
void AAbyssDiverCharacter::EquipSlot4() 
{ 
	if (bInputLockedByUI || bIsWorkingLocked) return;
	Server_SwitchToSlot(3); 
}
void AAbyssDiverCharacter::EquipSlot5() 
{ 
	if (bInputLockedByUI || bIsWorkingLocked) return;
	Server_SwitchToSlot(4); 
}



// E키를 눌렀을 때 실행되는 함수
void AAbyssDiverCharacter::TryInteract()
{
	if (bInputLockedByUI) return;
	if (bIsWorkingLocked) return;

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
			//IAbyssInteractionInterface::Execute_Interact(HitActor, this);

			// 서버 함수로 실행
			Server_TryInteract(HitActor);
		}
	}
}

void AAbyssDiverCharacter::Server_TryInteract_Implementation(AActor* TargetActor)
{
	if (CurrentWorkObject) return;

	if (!TargetActor) return;

	if (TargetActor->Implements<UAbyssInteractionInterface>())
	{
		IAbyssInteractionInterface::Execute_Interact(TargetActor, this);
	}
}

void AAbyssDiverCharacter::OnRep_Inventory()
{

	ApplyCurrentSlotVisual();

	if (IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI] OnRep_CurrentSlotIndex"));
		OnInventoryUpdated();
	}
}

void AAbyssDiverCharacter::OnRep_CurrentSlotIndex()
{
	UE_LOG(LogTemp, Warning, TEXT("[UI] OnRep_CurrentSlotIndex"));

	ApplyCurrentSlotVisual();

	if (IsLocallyControlled())
	{
		OnInventoryUpdated();
	}
}

bool AAbyssDiverCharacter::AddItemToInventory(AAbyssItemBase* ItemToAdd)
{
	if (!ItemToAdd) return false;

	// 인벤토리 배열(5칸)을 순회하면서 빈칸(nullptr) 찾기
	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		if (Inventory[i] == nullptr)
		{

			// 빈칸 발견! 아이템 넣기
			Inventory[i] = ItemToAdd;
			/*
			// 충돌 끄기
			ItemToAdd->SetActorEnableCollision(false);

			// 물리 시뮬레이션 끄기 
			UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(ItemToAdd->GetRootComponent());
			if (RootPrim)
			{
				RootPrim->SetSimulatePhysics(false);
				// 루트 매쉬 충돌 끄기
				RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				RootPrim->SetCollisionProfileName(TEXT("NoCollision"));
			}

			// 아이템을 카메라에 부착
			ItemToAdd->AttachToComponent(FirstPersonCameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);	

			// 액터 전체를 게임에서 숨기기
			if (CurrentSlotIndex == i) {
				ItemToAdd->SetActorHiddenInGame(false);
			}
			else
			{
				ItemToAdd->SetActorHiddenInGame(true);
			}

			// 소유권 설정
			//ItemToAdd->SetOwner(this);

			// UI 업데이트 알림
			if (IsLocallyControlled())
			{
				OnInventoryUpdated();
			}
			*/
			const bool bVisibleInHand = (CurrentSlotIndex == i);
			ItemToAdd->SetAsPickedUp(this, FirstPersonCameraComponent, bVisibleInHand);

			ApplyCurrentSlotVisual();

			if (IsLocallyControlled())
			{
				OnInventoryUpdated();
			}

			UE_LOG(LogTemp, Warning, TEXT("[Inventory] Picked item %s into slot %d"), *ItemToAdd->GetName(), i);

			return true; // 성공적으로 주움
		}
	}

	// 빈칸이 하나도 없음
	return false;
}

void AAbyssDiverCharacter::CheckForInteractables()
{
	if (!FirstPersonCameraComponent) return;

	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * InteractDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();

		// 맞은 물체가 인터페이스를 가지고 있다면?
		if (HitActor->Implements<UAbyssInteractionInterface>())
		{
			// 만약 이전에 쳐다보던 물체와 '다른' 물체라면?
			if (HitActor != FocusedActor)
			{
				// 기존 물체에게는 시선을 뗐다고 알려줌
				if (FocusedActor)
				{
					IAbyssInteractionInterface::Execute_OnLostFocus(FocusedActor);
				}

				// 새 물체에게는 시선이 닿았다고 알려줌
				FocusedActor = HitActor;
				IAbyssInteractionInterface::Execute_OnFocus(FocusedActor);
			}
			return; // 성공했으니 여기서 함수 종료
		}
	}

	// 허공을 보거나 상호작용 불가능한 벽을 보았을 때
	if (FocusedActor)
	{
		// 기존에 보던 물체의 UI를 꺼줌
		IAbyssInteractionInterface::Execute_OnLostFocus(FocusedActor);
		FocusedActor = nullptr; // 기억 지우기
	}
}

void AAbyssDiverCharacter::DropItem()
{
	if (bIsWorkingLocked) return;
	if (bInputLockedByUI) return;

	Server_DropItem();
}

void AAbyssDiverCharacter::Server_UseCurrentItem_Implementation()
{
	if (CurrentWorkObject) return;

	if (!Inventory.IsValidIndex(CurrentSlotIndex) || Inventory[CurrentSlotIndex] == nullptr)
	{
		return;
	}

	Inventory[CurrentSlotIndex]->UseItem();
}

void AAbyssDiverCharacter::Client_OnInventoryUpdated_Implementation()
{
	OnInventoryUpdated();
}

void AAbyssDiverCharacter::Server_DropItem_Implementation()
{
	if (CurrentWorkObject) return;

	if (Inventory.IsValidIndex(CurrentSlotIndex) && Inventory[CurrentSlotIndex] != nullptr)
	{
		AAbyssItemBase* ItemToDrop = Inventory[CurrentSlotIndex];
		Inventory[CurrentSlotIndex] = nullptr;

		if (FirstPersonCameraComponent)
		{
			const FVector DropLocation =
				FirstPersonCameraComponent->GetComponentLocation() +
				(FirstPersonCameraComponent->GetForwardVector() * 120.0f);

			const FRotator DropRotation = FirstPersonCameraComponent->GetComponentRotation();

			FVector ThrowDirection = FirstPersonCameraComponent->GetForwardVector();
			ThrowDirection.Z += 0.2f;
			ThrowDirection.Normalize();

			const float ThrowForce = 600.0f;
			const FVector ThrowImpulse = ThrowDirection * ThrowForce;

			if (AAbyssFlashLight* FlashLight = Cast<AAbyssFlashLight>(ItemToDrop))
			{
				FlashLight->SetLightEnabled(false);
			}

			ItemToDrop->SetAsDropped(DropLocation, DropRotation, ThrowImpulse);

			if (IsLocallyControlled())
			{
				OnInventoryUpdated();
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Throw Item: %s"), *ItemToDrop->ItemName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Abyss] Empty Slot"));
	}
}

// 인터페이스 필수 구현: 심장(컴포넌트) 반환
UAbilitySystemComponent* AAbyssDiverCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// [서버 측 초기화] 캐릭터가 컨트롤러에 빙의(Possess)될 때
void AAbyssDiverCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		// GAS 초기화의 핵심 함수: (소유자 액터, 물리적 아바타 액터)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	SetupEnhancedInput();
}

// 산소 값이 변할 때마다 엔진이 알아서 호출해 주는 함수
void AAbyssDiverCharacter::OnOxygenChangedCallback(const FOnAttributeChangeData& Data)
{
	// 블루프린트(UI) 쪽으로 "산소 변했다!" 하고 현재값과 최대값을 방송(Broadcast)합니다.
	OnOxygenChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxOxygen());
}

// 체력 값이 변할 때마다 엔진이 알아서 호출해 주는 함수
void AAbyssDiverCharacter::OnHealthChangedCallback(const FOnAttributeChangeData& Data)
{
	// 블루프린트(UI) 쪽으로 "체력 변했다!" 하고 현재값과 최대값을 방송(Broadcast)합니다.
	OnHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());
}

// 배터리 값이 변할 때마다 엔진이 알아서 호출해 주는 함수
void AAbyssDiverCharacter::OnBatteryChangedCallback(const FOnAttributeChangeData& Data)
{
	// 블루프린트(UI) 쪽으로 "체력 변했다!" 하고 현재값과 최대값을 방송(Broadcast)합니다.
	OnBatteryChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxBattery());
}

void AAbyssDiverCharacter::SetupEnhancedInput()
{
	if (!DefaultMappingContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Input] DefaultMappingContext is null"));
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Input] PlayerController is null"));
		return;
	}

	if (!PlayerController->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Input] Not Local Controller"));
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Input] LocalPlayer is null"));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Input] EnhancedInput Subsystem is null"));
		return;
	}

	Subsystem->AddMappingContext(DefaultMappingContext, 0);
	UE_LOG(LogTemp, Warning, TEXT("[Input] MappingContext Added"));

	// 로비에서 넘어올 때 UI 입력 모드가 남아있을 수 있으므로 복원
	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = false;
}

void AAbyssDiverCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	SetupEnhancedInput();
}

bool AAbyssDiverCharacter::HasEmptyInventorySlot() const
{
	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		if (Inventory[i] == nullptr)
		{
			return true; // 빈칸 있음!
		}
	}
	return false; // 꽉 참
}

void AAbyssDiverCharacter::Server_AcceptMissionById_Implementation(FName MissionId)
{
	if (AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>())
	{
		GM->AcceptMissionById(MissionId);
	}
}

void AAbyssDiverCharacter::SetInsideSubmarine(bool bInside)
{
	UCharacterMovementComponent* MoveComp = Cast<UCharacterMovementComponent>(GetCharacterMovement());
	if (!MoveComp) return;

	if (bInside)
	{
		// 1. 수영 능력 강제 차단 (물 속 물리 볼륨을 무시하게 됨)
		MoveComp->NavAgentProps.bCanSwim = false;

		// 2. 강제로 걷기/낙하 모드로 변경 (수중에서도 중력이 적용되어 바닥으로 떨어짐)
		MoveComp->SetMovementMode(MOVE_Falling);
		IsSwimming = false;
	}
	else
	{
		// 1. 잠수함 밖으로 나가면 수영 능력 복구
		MoveComp->NavAgentProps.bCanSwim = true;

		// 2. 만약 현재 맵의 물 볼륨 안에 있다면 즉시 수영 모드로 전환
		if (MoveComp->IsInWater())
		{
			MoveComp->SetMovementMode(MOVE_Swimming);
			IsSwimming = true;
		}
	}
}

void AAbyssDiverCharacter::SetCurrentWorkObject(AAbyssMissionWorkObject* WorkObject)
{
	CurrentWorkObject = WorkObject;
}

void AAbyssDiverCharacter::ClearCurrentWorkObject(AAbyssMissionWorkObject* WorkObject)
{
	if (CurrentWorkObject == WorkObject)
	{
		CurrentWorkObject = nullptr;
	}
}

void AAbyssDiverCharacter::CancelCurrentWorkByEnemyAttack()
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Warning, TEXT("[Diver] CancelCurrentWorkByEnemyAttack Called"));

	if (CurrentWorkObject)
	{
		CurrentWorkObject->CancelWork();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Diver] CurrentWorkObject is NULL"));
	}
}

void AAbyssDiverCharacter::SetInputLockedByUI(bool bLocked)
{
	bInputLockedByUI = bLocked;
}
