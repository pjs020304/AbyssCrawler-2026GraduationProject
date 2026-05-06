#include "AbyssSubmarine.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h" // [추가됨]
#include "AbyssDiverCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

AAbyssSubmarine::AAbyssSubmarine()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	DescentSpeed = 200.0f;
	bIsDescending = false;

	// 1. 최상위 루트 컴포넌트 생성
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 2. 잠수함 메쉬 생성 및 조립
	SubmarineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SubmarineMesh"));
	SubmarineMesh->SetupAttachment(RootComponent);
	// 외형 메쉬는 충돌만 처리하고 레이캐스트(상호작용)를 막지 않게 설정할 수도 있습니다.

	// 3. 콘솔 메쉬 생성 및 조립
	ConsoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleMesh"));
	ConsoleMesh->SetupAttachment(SubmarineMesh); // 잠수함 내부에 배치되도록 설정
	// 상호작용 레이저에 맞아야 하므로 Block 설정
	ConsoleMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 4. 탑승 확인용 박스 설정
	InteriorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteriorVolume"));
	InteriorVolume->SetupAttachment(SubmarineMesh); // 잠수함 메쉬를 따라다니도록 설정
	InteriorVolume->SetBoxExtent(FVector(300.0f, 200.0f, 200.0f));
	// 겹침만 허용하고 물리적 충돌은 무시
	InteriorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteriorVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	InteriorVolume->OnComponentBeginOverlap.AddDynamic(this, &AAbyssSubmarine::OnInteriorOverlapBegin);
	InteriorVolume->OnComponentEndOverlap.AddDynamic(this, &AAbyssSubmarine::OnInteriorOverlapEnd);
}

void AAbyssSubmarine::BeginPlay()
{
	Super::BeginPlay();
}

void AAbyssSubmarine::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;
	if (bIsDescending) return;

	if (AreAllPlayersBoarded())
	{
		StartDescent();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Have to ride all Players"));
	}
}

// (선택) 콘솔을 쳐다볼 때 텍스트 띄우기 (필요시 구현)
void AAbyssSubmarine::OnFocus_Implementation() {}
void AAbyssSubmarine::OnLostFocus_Implementation() {}

bool AAbyssSubmarine::AreAllPlayersBoarded()
{
	AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS) return false;

	int32 TotalPlayers = GS->PlayerArray.Num();

	TArray<AActor*> OverlappingDivers;
	InteriorVolume->GetOverlappingActors(OverlappingDivers, AAbyssDiverCharacter::StaticClass());

	return OverlappingDivers.Num() >= TotalPlayers;
}

// 새로운 함수들 구현부 추가
void AAbyssSubmarine::OnInteriorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(OtherActor))
	{
		// 서버와 클라이언트 모두 예측을 위해 실행합니다.
		Diver->SetInsideSubmarine(true);
	}
}

void AAbyssSubmarine::OnInteriorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(OtherActor))
	{
		Diver->SetInsideSubmarine(false);
	}
}

void AAbyssSubmarine::StartDescent()
{
	bIsDescending = true;
}

void AAbyssSubmarine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bIsDescending)
	{
		FVector CurrentLoc = GetActorLocation();
		FVector NewLoc = UKismetMathLibrary::VInterpTo_Constant(CurrentLoc, TargetLocation, DeltaTime, DescentSpeed);

		// bSweep=true 옵션을 추가하여 바닥이나 지형을 통과해버리지 않고 물리 충돌을 연산하게 합니다.
		SetActorLocation(NewLoc, true);

		if (FVector::Dist(NewLoc, TargetLocation) < 10.0f)
		{
			bIsDescending = false;

		}
	}
}

void AAbyssSubmarine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssSubmarine, bIsDescending);
}