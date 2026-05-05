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

	// 2. [추가됨] 잠수함 메쉬 생성 및 조립
	SubmarineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SubmarineMesh"));
	SubmarineMesh->SetupAttachment(RootComponent);
	// 외형 메쉬는 충돌만 처리하고 레이캐스트(상호작용)를 막지 않게 설정할 수도 있습니다.

	// 3. [추가됨] 콘솔 메쉬 생성 및 조립
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

void AAbyssSubmarine::StartDescent()
{
	bIsDescending = true;

	TArray<AActor*> OverlappingDivers;
	InteriorVolume->GetOverlappingActors(OverlappingDivers, AAbyssDiverCharacter::StaticClass());

	for (AActor* Diver : OverlappingDivers)
	{
		// [핵심] 잠수함 메쉬에 플레이어를 부착하여 안정적으로 같이 떨어지게 만듭니다.
		Diver->AttachToComponent(SubmarineMesh, FAttachmentTransformRules::KeepWorldTransform);
	}

}

void AAbyssSubmarine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bIsDescending)
	{
		FVector CurrentLoc = GetActorLocation();
		FVector NewLoc = UKismetMathLibrary::VInterpTo_Constant(CurrentLoc, TargetLocation, DeltaTime, DescentSpeed);
		SetActorLocation(NewLoc);

		if (FVector::Dist(NewLoc, TargetLocation) < 10.0f)
		{
			bIsDescending = false;
			UE_LOG(LogTemp, Warning, TEXT("심해 지점에 도착했습니다."));

			// 도착 시 부착 해제
			TArray<AActor*> OverlappingDivers;
			InteriorVolume->GetOverlappingActors(OverlappingDivers, AAbyssDiverCharacter::StaticClass());
			for (AActor* Diver : OverlappingDivers)
			{
				Diver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			}
		}
	}
}

void AAbyssSubmarine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssSubmarine, bIsDescending);
}