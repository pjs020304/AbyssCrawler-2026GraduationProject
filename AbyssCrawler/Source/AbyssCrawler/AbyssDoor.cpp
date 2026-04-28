#include "AbyssDoor.h"
#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

AAbyssDoor::AAbyssDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
	DoorFrame->SetupAttachment(RootScene);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorFrame);

	bIsOpen = false;
    DirectionMultiplier = 1.0f;
	CurrentYaw = 0.0f;
}

void AAbyssDoor::BeginPlay()
{
	Super::BeginPlay();
	CurrentYaw = DoorMesh->GetRelativeRotation().Yaw;
}

// [핵심] 상호작용이 들어오면 그냥 상태만 뒤집습니다.
void AAbyssDoor::Interact_Implementation(AActor* InstigatorActor)
{
    if (!HasAuthority()) return;
    if (!InstigatorActor) return;

    if (!bIsOpen) // 문이 닫혀있어서 '열어야' 할 때만 방향을 계산합니다.
    {
        // 1. 문의 정면 방향 벡터 구하기
        FVector DoorForward = GetActorRightVector();

        // 2. 문에서 플레이어를 향하는 벡터 구하기
        FVector DirToPlayer = InstigatorActor->GetActorLocation() - GetActorLocation();
        DirToPlayer.Normalize(); // 방향만 필요하므로 길이를 1로 정규화

        // 3. 두 벡터의 내적(Dot Product: 얼마나 같은 방향인가) 계산
        float DotResult = FVector::DotProduct(DoorForward, DirToPlayer);

        /*
        
        // 4. 플레이어 위치에 따라 문이 열릴 방향(부호) 결정
        if (DotResult > 0)
        {
            // 플레이어가 문 앞에 있음 -> 문을 밀어내려면 음수 방향으로 회전
            DirectionMultiplier = -1.0f;
        }
        else
        {
            // 플레이어가 문 뒤에 있음 -> 문을 밀어내려면 양수 방향으로 회전
            DirectionMultiplier = 1.0f;
        }

        */
    }

    // 상태 뒤집기 (열림 <-> 닫힘)
    bIsOpen = !bIsOpen;

	// 여기에 문 열리는 소리 추가 가능
	// UGameplayStatics::PlaySoundAtLocation(this, DoorSound, GetActorLocation());
}

void AAbyssDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAbyssDoor, bIsOpen);
    DOREPLIFETIME(AAbyssDoor, DirectionMultiplier);
}

void AAbyssDoor::OnRep_DoorState()
{
}

void AAbyssDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    float TargetAngle = bIsOpen ? (OpenAngle * DirectionMultiplier) : 0.0f;

	if (!FMath::IsNearlyEqual(CurrentYaw, TargetAngle, 0.1f))
	{
		CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetAngle, DeltaTime, 5.0f);
		FRotator NewRotation = FRotator(0.0f, CurrentYaw, 0.0f);
		DoorMesh->SetRelativeRotation(NewRotation);
	}
}