#include "AbyssDiverCharacter.h"
#include "AbyssOctopusCharacter.h"
#include "AbyssCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystemComponent.h"
#include "AbyssAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AbyssFlashLight.h"
#include "AbyssCorpseItem.h"
#include "AbyssGameMode.h"
#include "AbyssPlayerState.h"
#include "MainHUDWidget.h"
#include "Mission/UI/MissionSelectUIWidget.h"
#include "Mission/Contents/AbyssMissionSender.h"
#include "Blueprint/UserWidget.h"
#include "Mission/Contents/AbyssMissionWorkObject.h"
#include "Particles/ParticleSystemComponent.h"
#include "EngineUtils.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/PhysicsVolume.h"

// 중요: 커스텀 무브먼트 컴포넌트 사용을 위한 생성자 선언
AAbyssDiverCharacter::AAbyssDiverCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAbyssCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.0f);

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), TEXT("Head"));

	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.f, 630.f));
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.0f, 0.f, -90.f));

	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	MarineSnowParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("MarineSnowParticleComponent"));
	MarineSnowParticleComponent->SetupAttachment(FirstPersonCameraComponent);
	MarineSnowParticleComponent->SetOnlyOwnerSee(false); // 다른 플레이어에게도 보이도록 설정

	GetMesh()->SetOwnerNoSee(false); 
	GetMesh()->bCastHiddenShadow = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UAbyssAttributeSet>(TEXT("AttributeSet"));
}

void AAbyssDiverCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 카메라를 Head 소켓에 부착 보장 (BP에서 Parent Socket이 비어 있어도 머리 본을 따라가도록)
	// SnapToTarget으로 상대 오프셋을 0으로 만들어 Head 본에 정확히 붙인다.
	// (기존 BP 오프셋은 루트 기준이라, 그대로 두면 머리 본 위에 더해져 위치가 어긋남)
	// 위치는 머리 본을 따라가고, 회전은 bUsePawnControlRotation에 의해 마우스로만 제어됨.
	if (FirstPersonCameraComponent && GetMesh())
	{
		FirstPersonCameraComponent->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("Head"));

		// 머리에서 미세 조정이 필요하면 여기서 오프셋을 명시적으로 설정
		FirstPersonCameraComponent->SetRelativeLocation(FVector::ZeroVector);
	}

	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		// 파이썬 스크립트로 생성한 볼륨이거나 언바운드 볼륨 캐싱
		if (It->GetActorNameOrLabel().Contains(TEXT("DeepSea")) || It->bUnbound)
		{
			DeepSeaPPVolume = *It;
			//UE_LOG(LogTemp, Warning, TEXT("Get Deep Sea PP Volume: %s"), *DeepSeaPPVolume->GetDebugName());
			break;
		}
	}
	
	APhysicsVolume* CurrentVolume = GetCharacterMovement() ? GetCharacterMovement()->GetPhysicsVolume() : nullptr;
	bool bInWater = CurrentVolume && CurrentVolume->bWaterVolume;

	if (bInWater)
	{
		CurrentPPWeight = 1.0f;
	}
	else
	{
		CurrentPPWeight = 0.0f;
	}
	
	if (DeepSeaPPVolume)
	{
		DeepSeaPPVolume->BlendWeight = CurrentPPWeight;
	}

	if (AbilitySystemComponent && AttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetOxygenAttribute()).AddUObject(this, &AAbyssDiverCharacter::OnOxygenChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AAbyssDiverCharacter::OnHealthChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBatteryAttribute()).AddUObject(this, &AAbyssDiverCharacter::OnBatteryChangedCallback);

		if (OxygenDrainEffectClass)
		{
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddInstigator(this, this);

			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(OxygenDrainEffectClass, 1.0f, Context);

			if (SpecHandle.IsValid())
			{

				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// Enhanced Input 留ㅽ븨
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
		Inventory.Init(nullptr, 5);
		CurrentSlotIndex = 0;

		if (DefaultItemClass)
		{
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
				// ?몃깽?좊━ 0踰??щ’???ｊ린
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

				NewItem->AttachToComponent(FirstPersonCameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

				NewItem->SetActorEnableCollision(false);
				*/

				CurrentSlotIndex = 0;

				NewItem->SetAsPickedUp(this, GetMesh(), true, HandSocketName);
				ApplyCurrentSlotVisual();

				if (IsLocallyControlled())
				{
					OnInventoryUpdated();
				}
				
			}
		}
	}
		
	if (IsLocallyControlled())
	{
		if (FadeWidgetClass)
		{
			FadeWidget = CreateWidget<UUserWidget>(GetWorld(), FadeWidgetClass);

			if (FadeWidget)
			{
				FadeWidget->AddToViewport(10000);
			}
		}
	}

}

void AAbyssDiverCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Post Process Blending 로직
	APhysicsVolume* CurrentVolume = GetCharacterMovement() ? GetCharacterMovement()->GetPhysicsVolume() : nullptr;
	bool bInWater = CurrentVolume && CurrentVolume->bWaterVolume;

	if (DeepSeaPPVolume)
	{
		TargetPPWeight = bInWater ? 1.0f : 0.0f;
		CurrentPPWeight = FMath::FInterpTo(CurrentPPWeight, TargetPPWeight, DeltaTime, PPBlendSpeed);
		DeepSeaPPVolume->BlendWeight = CurrentPPWeight;
	}

	// Update holding item state
	// Inventory / CurrentSlotIndex 는 복제되므로 모든 머신(시뮬레이션 프록시 포함)에서 계산해야
	// 다른 클라이언트가 볼 때도 들기 애니메이션이 올바르게 재생된다.
	bIsHoldingItem = Inventory.IsValidIndex(CurrentSlotIndex) && Inventory[CurrentSlotIndex] != nullptr;

	// Marine Snow 동적 파라미터 제어
	if (MarineSnowParticleComponent)
	{
		if (bInWater && !MarineSnowParticleComponent->IsActive())
		{
			MarineSnowParticleComponent->Activate();
		}
		else if (!bInWater && MarineSnowParticleComponent->IsActive())
		{
			MarineSnowParticleComponent->Deactivate();
		}

		float Speed = GetVelocity().Size();
		// 최대 이동 속도를 600.0f 정도로 가정하여 비율 계산
		float SpeedRatio = FMath::Clamp(Speed / 600.0f, 0.0f, 1.0f);
		
		// 멈춰 있을 때 1.0, 빠르게 이동할 때 5.0 배속/스폰율
		float SpawnRateMult = FMath::Lerp(1.0f, 5.0f, SpeedRatio);
		// 난기류 강도를 1.0에서 3.0으로 증가시켜 격렬한 흩날림 표현
		float TurbulenceMult = FMath::Lerp(1.0f, 3.0f, SpeedRatio);
		
		// 파티클 시스템 인스턴스 파라미터 연동
		MarineSnowParticleComponent->SetFloatParameter(FName(TEXT("SpeedMultiplier")), SpawnRateMult);
		MarineSnowParticleComponent->SetFloatParameter(FName(TEXT("Turbulence")), TurbulenceMult);
	}

	CheckForInteractables();

	// Multiplayer: Sync camera rotation for simulated proxies to replicate look direction and flashlight direction
	if (!IsLocallyControlled() && FirstPersonCameraComponent)
	{
		FRotator ReplicatedAimRot = GetBaseAimRotation();
		FirstPersonCameraComponent->SetWorldRotation(ReplicatedAimRot);
	}
}

void AAbyssDiverCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (AbilitySystemComponent)
	{
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

		// ?꾩씠???ъ슜 (留덉슦??醫뚰겢由?
		EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::UseCurrentItem);
		EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Completed, this, &AAbyssDiverCharacter::StopUseCurrentItem);
		EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Canceled, this, &AAbyssDiverCharacter::StopUseCurrentItem);

		// ?щ’ 蹂寃?(?レ옄??1, 2)
		EnhancedInputComponent->BindAction(Slot1Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot1);
		EnhancedInputComponent->BindAction(Slot2Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot2);
		EnhancedInputComponent->BindAction(Slot3Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot3);
		EnhancedInputComponent->BindAction(Slot4Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot4);
		EnhancedInputComponent->BindAction(Slot5Action, ETriggerEvent::Started, this, &AAbyssDiverCharacter::EquipSlot5);

		// ?곹샇?묒슜 Interaction
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::TryInteract);

		// 踰꾨━湲?
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::DropItem);

		// Chat
		EnhancedInputComponent->BindAction(ChatAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::OpenChatInput);

		EnhancedInputComponent->BindAction(MissionUIAction, ETriggerEvent::Started, this, &AAbyssDiverCharacter::ToggleMissionPanel);
	}
}

void AAbyssDiverCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbyssDiverCharacter, Inventory);
	DOREPLIFETIME(AAbyssDiverCharacter, CurrentSlotIndex);
	DOREPLIFETIME(AAbyssDiverCharacter, CurrentWorkType);
	DOREPLIFETIME(AAbyssDiverCharacter, GhostHauntStage);
}

void AAbyssDiverCharacter::StartDash()
{
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

void AAbyssDiverCharacter::Client_OnSwarmBite_Implementation(FVector SwarmCenter)
{
	// 연출(카메라 셰이크/화면 효과)은 블루프린트의 OnSwarmBiteFeedback에서 구현.
	OnSwarmBiteFeedback(SwarmCenter);
}

// --- [유령(원혼) 디버프 / 포획] ---

void AAbyssDiverCharacter::SetGhostHauntStage(int32 NewStage)
{
	// 서버 권위. 포획되어 죽는 중이면 단계 변경 무시.
	if (!HasAuthority() || bCaughtByGhost)
	{
		return;
	}

	NewStage = FMath::Clamp(NewStage, 0, 3);
	if (GhostHauntStage == NewStage)
	{
		return;
	}

	GhostHauntStage = NewStage;

	// 서버에서는 OnRep이 자동 호출되지 않으므로 직접 적용.
	ApplyGhostHauntMovement();
	OnGhostHauntStageChanged(GhostHauntStage);
}

void AAbyssDiverCharacter::OnRep_GhostHauntStage()
{
	ApplyGhostHauntMovement();
	OnGhostHauntStageChanged(GhostHauntStage);
}

void AAbyssDiverCharacter::ApplyGhostHauntMovement()
{
	if (UAbyssCharacterMovementComponent* MyCMC =
			Cast<UAbyssCharacterMovementComponent>(GetCharacterMovement()))
	{
		const float Mul = GhostStageSpeedMultipliers.IsValidIndex(GhostHauntStage)
							  ? GhostStageSpeedMultipliers[GhostHauntStage]
							  : 1.f;
		MyCMC->HauntSpeedMultiplier = Mul;
	}
}

void AAbyssDiverCharacter::CaughtByGhost()
{
	if (!HasAuthority() || bCaughtByGhost || bIsDead)
	{
		return;
	}

	bCaughtByGhost = true;

	// 모든 머신에 포획 상태/암전 연출 적용
	Multicast_CaughtByGhost();

	// 암전 후 사망 (기존 사망 파이프라인 재사용)
	GetWorldTimerManager().SetTimer(
		GhostKillTimerHandle, this, &AAbyssDiverCharacter::Die,
		GhostKillDelay, /*bLoop=*/false);
}

void AAbyssDiverCharacter::Multicast_CaughtByGhost_Implementation()
{
	bCaughtByGhost = true;
	bIsGrabbed = true;

	// 이동/입력 정지
	if (UAbyssCharacterMovementComponent* MyCMC =
			Cast<UAbyssCharacterMovementComponent>(GetCharacterMovement()))
	{
		MyCMC->DisableMovement();
	}

	// 소유 클라이언트에서 완전 암전 연출
	if (IsLocallyControlled())
	{
		OnGhostKillBlackout();
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

void AAbyssDiverCharacter::ToggleMissionPanel()
{
	if (!IsLocallyControlled()) return;
	if (!MainHUDRef) return;
	if (bInputLockedByUI) return;
	if (bIsWorkingLocked) return;

	MainHUDRef->BP_ToggleMission();
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

void AAbyssDiverCharacter::Client_SetMovementBlocked_Implementation(bool bBlocked)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	PC->SetIgnoreMoveInput(bBlocked);

	if (bBlocked)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void AAbyssDiverCharacter::ConsumeItem(AAbyssItemBase* ItemToConsume)
{
	if (!ItemToConsume || !HasAuthority()) return;

	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		if (Inventory[i] == ItemToConsume)
		{
			Inventory[i] = nullptr;
			ItemToConsume->Destroy();
			
			// Refresh visual if we consumed the currently equipped item
			if (CurrentSlotIndex == i)
			{
				ApplyCurrentSlotVisual();
			}
			
			if (IsLocallyControlled())
			{
				OnInventoryUpdated();
			}
			else
			{
				Client_OnInventoryUpdated();
			}
			break;
		}
	}
}

void AAbyssDiverCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// [?듭떖] ?섏쁺 紐⑤뱶???뚮뒗 移대찓?쇨? 蹂대뒗 諛⑺뼢(Z異??ы븿)?쇰줈 ?대룞?댁빞 ??(6DOF)
		bool bIsSwimming = GetCharacterMovement()->IsSwimming();

		if (bIsSwimming && IsSwimming)
		{
			// ?섏쁺 以? ControlRotation???ъ슜?섏뿬 3李⑥썝 諛⑺뼢 ?띾뱷
			FRotator ControlRot = GetControlRotation();
			FVector ForwardDir = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
			FVector RightDir = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);

			AddMovementInput(ForwardDir, MovementVector.Y);
			AddMovementInput(RightDir, MovementVector.X);

			//UE_LOG(LogTemp, Warning, TEXT("Swiming now"))

		}
		else
		{
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
		AddMovementInput(FVector::UpVector, 1.0f);
		//UE_LOG(LogTemp, Warning, TEXT("Swiming Ascend now"))
		
	}
	else
	{
		// 嫄룰린 以?Space: ?먰봽
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
		// ?섏쁺 以?Ctrl: ?섏쭅 ?섍컯
		AddMovementInput(FVector::UpVector, -1.0f);
	}
	else
	{
		// 嫄룰린 以?Ctrl: ?낇겕由ш린 (異뷀썑 援ы쁽)
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

	if (bIsGrabbed)
	{
		CurrentEscapeClicks++;
		UE_LOG(LogTemp, Warning, TEXT("[Abyss] Escape Click! %d / %d"), CurrentEscapeClicks, RequiredEscapeClicks);
		if (CurrentEscapeClicks >= RequiredEscapeClicks)
		{
			EscapeGrab();
		}
		return;
	}

	if (!Inventory.IsValidIndex(CurrentSlotIndex) || Inventory[CurrentSlotIndex] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Abyss] Empty Slot"));
		return;
	}

	// [카메라 쉐이크] 반응성을 위해 서버 왕복을 기다리지 않고 입력 시점에 로컬로 재생.
	// (UseItem()은 서버 전용(HasAuthority)이라 거기 넣으면 클라 화면이 안 흔들린다)
	if (IsLocallyControlled())
	{
		if (const TSubclassOf<UCameraShakeBase> UseShake = Inventory[CurrentSlotIndex]->UseCameraShake)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->ClientStartCameraShake(UseShake);
			}
		}
	}

	// ?대씪???쒕쾭?먭쾶 ?ъ슜 ?붿껌留?蹂대깂
	if (!HasAuthority())
	{
		Server_UseCurrentItem();
		return;
	}

	// ?쒕쾭留??ㅼ젣 ?꾩씠???ъ슜
	Inventory[CurrentSlotIndex]->UseItem();
}

void AAbyssDiverCharacter::StopUseCurrentItem()
{
	if (bInputLockedByUI) return;
	if (bIsWorkingLocked) return;
	if (bIsGrabbed) return;

	if (!Inventory.IsValidIndex(CurrentSlotIndex) || Inventory[CurrentSlotIndex] == nullptr)
	{
		return;
	}

	if (!HasAuthority())
	{
		Server_StopUseCurrentItem();
		return;
	}

	Inventory[CurrentSlotIndex]->EndUseItem();
}

void AAbyssDiverCharacter::SwitchToSlot(int32 NewIndex)
{
	/*
	if (CurrentSlotIndex == NewIndex) return;

	// 湲곗〈 ?꾩씠???④린湲??깆쓽 濡쒖쭅???꾩슂?섎떎硫??ш린???묒꽦
	// ?? Inventory[CurrentSlotIndex]->SetActorHiddenInGame(true);
	if (Inventory.IsValidIndex(CurrentSlotIndex) && Inventory[CurrentSlotIndex] != nullptr) 
	{
		Inventory[CurrentSlotIndex]->SetActorHiddenInGame(true);
	}
	CurrentSlotIndex = NewIndex;
	UE_LOG(LogTemp, Log, TEXT("Slot Change: %d"), CurrentSlotIndex + 1);

	// ???꾩씠??蹂댁씠湲?
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

	// 由ъ뒯 ?쒕쾭 ?몄뒪??UI 媛깆떊 蹂댁젙
	if (IsLocallyControlled())
	{
		OnInventoryUpdated();
	}
}

void AAbyssDiverCharacter::ApplyCurrentSlotVisual()
{
	TSet<AAbyssItemBase*> ProcessedItems;

	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		AAbyssItemBase* Item = Inventory[i];
		if (!Item)
		{
			continue;
		}

		if (ProcessedItems.Contains(Item))
		{
			continue;
		}

		ProcessedItems.Add(Item);

		bool bShouldBeVisible = false;

		for (int32 SlotIndex = 0; SlotIndex < Inventory.Num(); ++SlotIndex)
		{
			if (Inventory[SlotIndex] == Item && SlotIndex == CurrentSlotIndex)
			{
				bShouldBeVisible = true;
				break;
			}
		}

		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Item->GetRootComponent()))
		{
			RootPrim->SetSimulatePhysics(false);
			RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			RootPrim->SetCollisionProfileName(TEXT("NoCollision"));
		}

		Item->SetActorEnableCollision(false);

		// ?꾩옱 ?щ’ ?꾩씠?쒖? ?ㅼ떆 移대찓?쇱뿉 遺李?
		if (bShouldBeVisible && GetMesh())
		{
			Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			Item->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				HandSocketName
			);

			// [진단] 클라이언트에서 소켓 부착이 제대로 되는지 확인
			const TCHAR* NetMode =
				HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
			UE_LOG(LogTemp, Warning,
				TEXT("[EquipVisual:%s] Item=%s SocketExists=%d ItemLoc=%s MeshLoc=%s Hidden=%d"),
				NetMode,
				*Item->GetName(),
				GetMesh()->DoesSocketExist(HandSocketName) ? 1 : 0,
				*Item->GetActorLocation().ToString(),
				*GetMesh()->GetComponentLocation().ToString(),
				Item->IsHidden() ? 1 : 0);
		}

		// 숨겨지는 아이템은 NotifyUnequipped 호출 (가상 함수 → 각 아이템이 알아서 처리)
		if (!bShouldBeVisible)
		{
			Item->NotifyUnequipped();
		}

		Item->SetActorHiddenInGame(!bShouldBeVisible);
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



// E?ㅻ? ?뚮??????ㅽ뻾?섎뒗 ?⑥닔
void AAbyssDiverCharacter::TryInteract()
{
	if (bInputLockedByUI) return;
	if (bIsWorkingLocked) return;

	if (!FirstPersonCameraComponent) return;

	// 1. ?덉씠罹먯뒪???쒖옉??移대찓???꾩튂)怨??앹젏(移대찓?쇨? 諛붾씪蹂대뒗 諛⑺뼢 * 嫄곕━) 怨꾩궛
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * InteractDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // ??罹먮┃?곕뒗 寃?ъ뿉???쒖쇅

	// 2. ?덉뿉 蹂댁씠??臾쇱껜(ECC_Visibility: 湲곕낯?쇰줈 StaticMesh Block, Ignore遺遺??ㅼ젙 媛??瑜?湲곗??쇰줈 ?덉씠? ?섍린
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);

	// (?붾쾭洹몄슜)
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);

	// 3. ?쇱씤?몃젅?댁뒪??嫄몃졇????
	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();

		// 4. 洹?臾쇱껜媛 ?곹샇?묒슜 ?명꽣?섏씠?ㅻ? ?곸냽諛쏆븯?붿? ?뺤씤
		if (HitActor->Implements<UAbyssInteractionInterface>())
		{
			// 5. ?명꽣?섏씠?ㅼ쓽 Interact ?⑥닔 ?ㅽ뻾 
			//IAbyssInteractionInterface::Execute_Interact(HitActor, this);

			// ?쒕쾭 ?⑥닔濡??ㅽ뻾
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
	for (AAbyssItemBase* Item : Inventory)
	{
		if (Item == ItemToAdd)
		{
			return false;
		}
	}

	if (!ItemToAdd) return false;

	// ?몃깽?좊━ 諛곗뿴(5移????쒗쉶?섎㈃??鍮덉뭏(nullptr) 李얘린
	if (AAbyssCorpseItem* CorpseItem = Cast<AAbyssCorpseItem>(ItemToAdd))
	{
		if (!HasEnoughEmptyInventorySlots(CorpseItem->SlotCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Inventory] Not enough slots for corpse"));
			return false;
		}

		int32 FilledCount = 0;
		bool bFirstSlot = true;

		for (int32 i = 0; i < Inventory.Num(); ++i)
		{
			if (Inventory[i] == nullptr)
			{
				Inventory[i] = CorpseItem;
				FilledCount++;

				if (bFirstSlot)
				{
					CurrentSlotIndex = i;
					bFirstSlot = false;
				}

				if (FilledCount >= CorpseItem->SlotCost)
				{
					break;
				}
			}
		}

		CorpseItem->SetAsPickedUp(this, FirstPersonCameraComponent, true);

		ApplyCorpseCarryPenalty();
		ApplyCurrentSlotVisual();

		if (IsLocallyControlled())
		{
			OnInventoryUpdated();
		}

		UE_LOG(LogTemp, Warning, TEXT("[Inventory] Picked corpse item, SlotCost=%d"), CorpseItem->SlotCost);
		return true;

	}

	// 인벤토리 배열(5칸)을 순회하면서 빈칸(nullptr) 찾기
	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		if (Inventory[i] == nullptr)
		{

			// 鍮덉뭏 諛쒓껄! ?꾩씠???ｊ린
			Inventory[i] = ItemToAdd;

			/*
			// 異⑸룎 ?꾧린
			ItemToAdd->SetActorEnableCollision(false);

			// 臾쇰━ ?쒕??덉씠???꾧린 
			UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(ItemToAdd->GetRootComponent());
			if (RootPrim)
			{
				RootPrim->SetSimulatePhysics(false);
				// 猷⑦듃 留ㅼ돩 異⑸룎 ?꾧린
				RootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				RootPrim->SetCollisionProfileName(TEXT("NoCollision"));
			}

			// ?꾩씠?쒖쓣 移대찓?쇱뿉 遺李?
			ItemToAdd->AttachToComponent(FirstPersonCameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);	

			// ?≫꽣 ?꾩껜瑜?寃뚯엫?먯꽌 ?④린湲?
			if (CurrentSlotIndex == i) {
				ItemToAdd->SetActorHiddenInGame(false);
			}
			else
			{
				ItemToAdd->SetActorHiddenInGame(true);
			}

			// ?뚯쑀沅??ㅼ젙
			//ItemToAdd->SetOwner(this);

			// UI ?낅뜲?댄듃 ?뚮┝
			if (IsLocallyControlled())
			{
				OnInventoryUpdated();
			}
			*/
			const bool bVisibleInHand = (CurrentSlotIndex == i);
			ItemToAdd->SetAsPickedUp(this, GetMesh(), bVisibleInHand, HandSocketName);

			ApplyCurrentSlotVisual();

			if (IsLocallyControlled())
			{
				OnInventoryUpdated();
			}

			UE_LOG(LogTemp, Warning, TEXT("[Inventory] Picked item %s into slot %d"), *ItemToAdd->GetName(), i);

			return true; // ?깃났?곸쑝濡?二쇱?
		}
	}

	// 鍮덉뭏???섎굹???놁쓬
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

		// 留욎? 臾쇱껜媛 ?명꽣?섏씠?ㅻ? 媛吏怨??덈떎硫?
		if (HitActor->Implements<UAbyssInteractionInterface>())
		{
			// 留뚯빟 ?댁쟾??爾먮떎蹂대뜕 臾쇱껜? '?ㅻⅨ' 臾쇱껜?쇰㈃?
			if (HitActor != FocusedActor)
			{
				// 湲곗〈 臾쇱껜?먭쾶???쒖꽑???먮떎怨??뚮젮以?
				if (FocusedActor)
				{
					IAbyssInteractionInterface::Execute_OnLostFocus(FocusedActor);
				}

				// ??臾쇱껜?먭쾶???쒖꽑???우븯?ㅺ퀬 ?뚮젮以?
				FocusedActor = HitActor;
				IAbyssInteractionInterface::Execute_OnFocus(FocusedActor);
			}
			return; // ?깃났?덉쑝???ш린???⑥닔 醫낅즺
		}
	}

	// ?덇났??蹂닿굅???곹샇?묒슜 遺덇??ν븳 踰쎌쓣 蹂댁븯????
	if (FocusedActor)
	{
		// 湲곗〈??蹂대뜕 臾쇱껜??UI瑜?爰쇱쨲
		IAbyssInteractionInterface::Execute_OnLostFocus(FocusedActor);
		FocusedActor = nullptr; // 湲곗뼲 吏?곌린
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

void AAbyssDiverCharacter::Server_StopUseCurrentItem_Implementation()
{
	if (CurrentWorkObject) return;

	if (!Inventory.IsValidIndex(CurrentSlotIndex) || Inventory[CurrentSlotIndex] == nullptr)
	{
		return;
	}

	Inventory[CurrentSlotIndex]->EndUseItem();
}

void AAbyssDiverCharacter::Client_OnInventoryUpdated_Implementation()
{
	OnInventoryUpdated();
}

void AAbyssDiverCharacter::Server_DropItem_Implementation()
{
	if (CurrentWorkObject) return;

	if (!Inventory.IsValidIndex(CurrentSlotIndex) || Inventory[CurrentSlotIndex] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Abyss] Empty Slot"));
		return;
	}

	AAbyssItemBase* ItemToDrop = Inventory[CurrentSlotIndex];

	// 시체면 같은 포인터가 들어간 슬롯 전부 비우기
	if (AAbyssCorpseItem* CorpseItem = Cast<AAbyssCorpseItem>(ItemToDrop))
	{
		for (int32 i = 0; i < Inventory.Num(); ++i)
		{
			if (Inventory[i] == CorpseItem)
			{
				Inventory[i] = nullptr;
			}
		}

		RemoveCorpseCarryPenalty();
	}
	else
	{
		Inventory[CurrentSlotIndex] = nullptr;
	}

	bool bFoundNewSlot = false;

	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		if (Inventory[i] != nullptr)
		{
			CurrentSlotIndex = i;
			bFoundNewSlot = true;
			break;
		}
	}

	if (!bFoundNewSlot)
	{
		CurrentSlotIndex = 0;
	}

	if (FirstPersonCameraComponent)
	{
		const FVector DropLocation =
			FirstPersonCameraComponent->GetComponentLocation() +
			(FirstPersonCameraComponent->GetForwardVector() * 120.0f);

		FRotator DropRotation = FirstPersonCameraComponent->GetComponentRotation();

		// 시체는 회전 초기화
		if (Cast<AAbyssCorpseItem>(ItemToDrop))
		{
			DropRotation = FRotator::ZeroRotator;
		}

		FVector ThrowDirection = FirstPersonCameraComponent->GetForwardVector();
		ThrowDirection.Z += 0.2f;
		ThrowDirection.Normalize();

		const float ThrowForce = 600.0f;
		const FVector ThrowImpulse = ThrowDirection * ThrowForce;

		ItemToDrop->SetAsDropped(DropLocation, DropRotation, ThrowImpulse);

		if (AAbyssCorpseItem* CorpseItem = Cast<AAbyssCorpseItem>(ItemToDrop))
		{
			CorpseItem->SetActorRotation(FRotator::ZeroRotator);
		}

		if (AAbyssFlashLight* FlashLight = Cast<AAbyssFlashLight>(ItemToDrop))
		{
			FlashLight->SetLightEnabled(false);
		}

		ItemToDrop->SetAsDropped(DropLocation, DropRotation, ThrowImpulse);
	}

	ApplyCurrentSlotVisual();

	if (IsLocallyControlled())
	{
		OnInventoryUpdated();
	}

	UE_LOG(LogTemp, Log, TEXT("Drop Item: %s"), *ItemToDrop->ItemName);
}

// ?명꽣?섏씠???꾩닔 援ы쁽: ?ъ옣(而댄룷?뚰듃) 諛섑솚
UAbilitySystemComponent* AAbyssDiverCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// [?쒕쾭 痢?珥덇린?? 罹먮┃?곌? 而⑦듃濡ㅻ윭??鍮숈쓽(Possess)????
void AAbyssDiverCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		// GAS 珥덇린?붿쓽 ?듭떖 ?⑥닔: (?뚯쑀???≫꽣, 臾쇰━???꾨컮? ?≫꽣)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	SetupEnhancedInput();
}

// ?곗냼 媛믪씠 蹂???뚮쭏???붿쭊???뚯븘???몄텧??二쇰뒗 ?⑥닔
void AAbyssDiverCharacter::OnOxygenChangedCallback(const FOnAttributeChangeData& Data)
{
	// 釉붾（?꾨┛??UI) 履쎌쑝濡?"?곗냼 蹂?덈떎!" ?섍퀬 ?꾩옱媛믨낵 理쒕?媛믪쓣 諛⑹넚(Broadcast)?⑸땲??
	OnOxygenChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxOxygen());
}

// 泥대젰 媛믪씠 蹂€???뚮쭏???붿쭊???뚯븘???몄텧??二쇰뒗 ?⑥닔
void AAbyssDiverCharacter::OnHealthChangedCallback(const FOnAttributeChangeData& Data)
{
	// 블루프린트(UI) 쪽으로 "체력 변했다!" 하고 현재값과 최대값을 방송(Broadcast)합니다.
	OnHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());

	// [카메라 쉐이크] 체력이 "감소"했고, 이 캐릭터가 "내 화면의 주인"일 때만 흔든다.
	// (IsLocallyControlled 체크가 없으면 리슨서버 호스트가 남의 피격에도 흔들린다)
	if (Data.NewValue < Data.OldValue && IsLocallyControlled() && HitCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			// 피해량에 비례한 강도 (기본: 1~30 피해 → 0.5~1.5 강도)
			const float Scale = FMath::GetMappedRangeValueClamped(
				HitShakeDamageRange, HitShakeScaleRange, Data.OldValue - Data.NewValue);
			PC->ClientStartCameraShake(HitCameraShake, Scale);
		}
	}

	if (Data.NewValue <= 0.0f && !bIsDead)
	{
		Die();
	}
}

// 諛고꽣由?媛믪씠 蹂€???뚮쭏???붿쭊???뚯븘???몄텧??二쇰뒗 ?⑥닔
void AAbyssDiverCharacter::OnBatteryChangedCallback(const FOnAttributeChangeData& Data)
{
	// 釉붾（?꾨┛??UI) 履쎌쑝濡?"泥대젰 蹂?덈떎!" ?섍퀬 ?꾩옱媛믨낵 理쒕?媛믪쓣 諛⑹넚(Broadcast)?⑸땲??
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

	// 濡쒕퉬?먯꽌 ?섏뼱????UI ?낅젰 紐⑤뱶媛 ?⑥븘?덉쓣 ???덉쑝誘濡?蹂듭썝
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
			return true; // 鍮덉뭏 ?덉쓬!
		}
	}
	return false; // 苑?李?
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
		// 1. ?섏쁺 ?λ젰 媛뺤젣 李⑤떒 (臾???臾쇰━ 蹂쇰ⅷ??臾댁떆?섍쾶 ??
		MoveComp->NavAgentProps.bCanSwim = false;

		// 2. 媛뺤젣濡?嫄룰린/?숉븯 紐⑤뱶濡?蹂寃?(?섏쨷?먯꽌??以묐젰???곸슜?섏뼱 諛붾떏?쇰줈 ?⑥뼱吏?
		MoveComp->SetMovementMode(MOVE_Falling);
		IsSwimming = false;
	}
	else
	{
		// 1. ?좎닔??諛뽰쑝濡??섍?硫??섏쁺 ?λ젰 蹂듦뎄
		MoveComp->NavAgentProps.bCanSwim = true;

		// 2. 留뚯빟 ?꾩옱 留듭쓽 臾?蹂쇰ⅷ ?덉뿉 ?덈떎硫?利됱떆 ?섏쁺 紐⑤뱶濡??꾪솚
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
	if (HasAuthority())
	{
		SetWorkType(EAbyssWorkType::MissionWork);
	}
}

void AAbyssDiverCharacter::ClearCurrentWorkObject(AAbyssMissionWorkObject* WorkObject)
{
	if (CurrentWorkObject == WorkObject)
	{
		CurrentWorkObject = nullptr;
		if (HasAuthority())
		{
			SetWorkType(EAbyssWorkType::None);
		}
	}
}

void AAbyssDiverCharacter::SetWorkType(EAbyssWorkType NewWorkType)
{
	if (HasAuthority())
	{
		CurrentWorkType = NewWorkType;
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

void AAbyssDiverCharacter::OnGrabbed(ACharacter* Grabber, int32 RequiredClicks, float DamagePerSecond)
{
	if (!HasAuthority())
	{
		Server_OnGrabbed(Grabber, RequiredClicks, DamagePerSecond);
		return;
	}

	Multicast_OnGrabbed(Grabber);

	RequiredEscapeClicks = RequiredClicks;
	GrabDamagePerSecond = DamagePerSecond;
	CurrentEscapeClicks = 0;
	
	// Start DOT timer
	GetWorld()->GetTimerManager().SetTimer(GrabDOTTimerHandle, this, &AAbyssDiverCharacter::ApplyGrabDamage, 1.0f, true);
}

void AAbyssDiverCharacter::Server_OnGrabbed_Implementation(ACharacter* Grabber, int32 RequiredClicks, float DamagePerSecond)
{
	OnGrabbed(Grabber, RequiredClicks, DamagePerSecond);
}

void AAbyssDiverCharacter::Multicast_OnGrabbed_Implementation(ACharacter* Grabber)
{
	bIsGrabbed = true;
	CurrentGrabber = Grabber;
	CurrentEscapeClicks = 0;
	
	if (UAbyssCharacterMovementComponent* MyCMC = Cast<UAbyssCharacterMovementComponent>(GetCharacterMovement()))
	{
		MyCMC->DisableMovement();
	}
}

void AAbyssDiverCharacter::EscapeGrab()
{
	if (!HasAuthority())
	{
		Server_EscapeGrab();
		return;
	}

	Multicast_EscapeGrab();
	GetWorld()->GetTimerManager().ClearTimer(GrabDOTTimerHandle);

	if (AAbyssOctopusCharacter* Octopus = Cast<AAbyssOctopusCharacter>(CurrentGrabber))
	{
		Octopus->OnGrabBroken();
	}
}

void AAbyssDiverCharacter::Server_EscapeGrab_Implementation()
{
	EscapeGrab();
}

void AAbyssDiverCharacter::Multicast_EscapeGrab_Implementation()
{
	bIsGrabbed = false;
	
	if (UAbyssCharacterMovementComponent* MyCMC = Cast<UAbyssCharacterMovementComponent>(GetCharacterMovement()))
	{
		MyCMC->bCheatFlying = false;
		
		if (IsSwimming || MyCMC->IsInWater())
		{
			MyCMC->SetMovementMode(MOVE_Swimming);
		}
		else
		{
			MyCMC->SetMovementMode(MOVE_Walking);
		}

		UE_LOG(LogTemp, Warning, TEXT("Multicast Escape Grab"));
	}
}

void AAbyssDiverCharacter::ApplyGrabDamage()
{
	if (HasAuthority() && bIsGrabbed && AbilitySystemComponent && AttributeSet)
	{
		float CurrentHealth = AttributeSet->GetHealth();
		AttributeSet->SetHealth(FMath::Clamp(CurrentHealth - GrabDamagePerSecond, 0.0f, AttributeSet->GetMaxHealth()));
	}
}

bool AAbyssDiverCharacter::HasEnoughEmptyInventorySlots(int32 NeededSlots) const
{
	int32 EmptyCount = 0;

	for (AAbyssItemBase* Item : Inventory)
	{
		if (Item == nullptr)
		{
			EmptyCount++;
		}
	}

	return EmptyCount >= NeededSlots;
}

void AAbyssDiverCharacter::ApplyCorpseCarryPenalty()
{
}

void AAbyssDiverCharacter::RemoveCorpseCarryPenalty()
{
}

void AAbyssDiverCharacter::OpenChatInput()
{
	if (!IsLocallyControlled()) return;
	if (!MainHUDRef) return;

	MainHUDRef->BP_OpenChat();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	TSharedPtr<SWidget> ChatInputWidget = MainHUDRef->GetChatInputTextObject();
	if (!ChatInputWidget.IsValid()) return;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ChatInputWidget);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	SetInputLockedByUI(true);
}

void AAbyssDiverCharacter::SendChatMessage(const FString& Message)
{
	const FString TrimmedMessage = Message.TrimStartAndEnd();
	if (TrimmedMessage.IsEmpty()) return;

	Server_SendChatMessage(TrimmedMessage);
}

void AAbyssDiverCharacter::Client_ReceiveChatMessage_Implementation(const FString& Message)
{
	if (MainHUDRef)
	{
		MainHUDRef->AddChatMessage(Message);
	}
}

void AAbyssDiverCharacter::Server_SendChatMessage_Implementation(const FString& Message)
{
	FString SenderName = TEXT("Player");

	if (AAbyssPlayerState* PS = GetPlayerState<AAbyssPlayerState>())
	{
		SenderName = PS->Nickname.ToString();
	}

	const FString FinalMessage =
		FString::Printf(TEXT("%s : %s"), *SenderName, *Message);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(PC->GetPawn()))
			{
				Diver->Client_ReceiveChatMessage(FinalMessage);
			}
		}
	}
}



void AAbyssDiverCharacter::Die()
{
	if (HasAuthority())
	{
		Server_Die_Implementation();
	}
	else
	{
		Server_Die();
	}
}

void AAbyssDiverCharacter::Server_Die_Implementation()
{
	if (bIsDead) return;
	bIsDead = true;

	// Spawn Corpse Item
	if (AAbyssCorpseItem* Corpse = GetWorld()->SpawnActor<AAbyssCorpseItem>(AAbyssCorpseItem::StaticClass(), GetActorLocation(), GetActorRotation()))
	{
		Corpse->SetDeadPlayerState(GetPlayerState());
		Corpse->InitCorpse(GetMesh()->GetSkeletalMeshAsset(), GetMesh()->GetMaterials());
	}

	// Inform GameMode
	if (AAbyssGameMode* GM = Cast<AAbyssGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->OnPlayerDied(GetController());
	}

	// Unpossess and set to spectator
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ChangeState(NAME_Spectating);
		PC->ClientGotoState(NAME_Spectating);
	}

	Multicast_Die();
}

void AAbyssDiverCharacter::Multicast_Die_Implementation()
{
	// Hide character mesh and disable collision
	GetMesh()->SetVisibility(false, true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
