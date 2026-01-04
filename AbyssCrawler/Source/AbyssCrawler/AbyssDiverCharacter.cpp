#include "AbyssDiverCharacter.h"
#include "AbyssCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
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