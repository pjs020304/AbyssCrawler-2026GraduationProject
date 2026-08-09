#include "AbyssSubmarine.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h" // [추가됨]
#include "AbyssDiverCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "AbyssGameMode.h"
#include "AbyssGameState.h"

AAbyssSubmarine::AAbyssSubmarine()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	DescentSpeed = 200.0f;
	bIsDescending = false;
	bIsAscending = false;

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

	// 텍스트 컴포넌트 추가
	InteractionText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InteractionText"));
	InteractionText->SetupAttachment(ConsoleMesh);
	InteractionText->SetHorizontalAlignment(EHTA_Center);
	InteractionText->SetVerticalAlignment(EVRTA_TextCenter);
	InteractionText->SetText(FText::GetEmpty());
	InteractionText->SetVisibility(false);
	InteractionText->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	InteractionText->SetTextRenderColor(FColor::Cyan);

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
	InitialLocation = GetActorLocation();
}

void AAbyssSubmarine::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;
	if (bIsDescending || bIsAscending) return;

	if (AreAllPlayersBoarded())
	{
		if (FVector::Dist(GetActorLocation(), TargetLocation) < 50.0f)
		{
			StartAscent();
		}
		else
		{
			StartDescent();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Have to ride all Players"));
	}
}

// 콘솔을 쳐다볼 때 텍스트 띄우기 (필요시 구현)
void AAbyssSubmarine::OnFocus_Implementation()
{
	if (InteractionText)
	{
		UpdateInteractionText();
		InteractionText->SetVisibility(true);
	}
}

void AAbyssSubmarine::OnLostFocus_Implementation()
{
	if (InteractionText)
	{
		InteractionText->SetVisibility(false);
	}
}

void AAbyssSubmarine::UpdateInteractionText()
{
	AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS) return;

	int32 TotalPlayers = GS->PlayerArray.Num();
	if (TotalPlayers == 0) TotalPlayers = 1;

	TArray<AActor*> OverlappingDivers;
	InteriorVolume->GetOverlappingActors(OverlappingDivers, AAbyssDiverCharacter::StaticClass());
	int32 BoardedPlayers = OverlappingDivers.Num();

	FString StateString = TEXT("Ready to Descend");
	if (FVector::Dist(GetActorLocation(), TargetLocation) < 50.0f)
	{
		StateString = TEXT("Ready to Return");
	}

	FString DisplayString = FString::Printf(TEXT("Boarded: %d / %d\n%s"), BoardedPlayers, TotalPlayers, *StateString);
	InteractionText->SetText(FText::FromString(DisplayString));

	UE_LOG(LogTemp, Warning, TEXT("UpdateSubmarineText"));
}

bool AAbyssSubmarine::AreAllPlayersBoarded()
{
	AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS) return false;

	int32 TotalPlayers = GS->PlayerArray.Num();
	if (TotalPlayers == 0) TotalPlayers = 1;

	TArray<AActor*> OverlappingDivers;
	InteriorVolume->GetOverlappingActors(OverlappingDivers, AAbyssDiverCharacter::StaticClass());

	return OverlappingDivers.Num() >= TotalPlayers;
}

void AAbyssSubmarine::TryClearGameByBoarding()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!AreAllPlayersBoarded())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Submarine] Not all players boarded"));
		return;
	}

	AAbyssGameMode* GM = GetWorld()->GetAuthGameMode<AAbyssGameMode>();
	if (!GM)
	{
		return;
	}

	GM->OnPlayerEscaped(nullptr);
}

// 새로운 함수들 구현부 추가
void AAbyssSubmarine::OnInteriorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(OtherActor))
	{
		// 서버와 클라이언트 모두 예측을 위해 실행합니다.
		Diver->SetInsideSubmarine(true);

		if (InteractionText && InteractionText->IsVisible())
		{
			UpdateInteractionText();
		}

		// 서버 클리어 판정
		if (HasAuthority())
		{
			TryClearGameByBoarding();
		}
	}
}

void AAbyssSubmarine::OnInteriorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(OtherActor))
	{
		Diver->SetInsideSubmarine(false);

		if (InteractionText && InteractionText->IsVisible())
		{
			UpdateInteractionText();
		}
	}
}

void AAbyssSubmarine::StartDescent()
{
	bIsDescending = true;
	bIsAscending = false;
}

void AAbyssSubmarine::StartAscent()
{
	bIsAscending = true;
	bIsDescending = false;
}

void AAbyssSubmarine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (bIsDescending)
		{
			FVector CurrentLoc = GetActorLocation();
			FVector NewLoc = UKismetMathLibrary::VInterpTo_Constant(CurrentLoc, TargetLocation, DeltaTime, DescentSpeed);

			SetActorLocation(NewLoc, true);

			if (FVector::Dist(NewLoc, TargetLocation) < 10.0f)
			{
				bIsDescending = false;
			}
		}
		else if (bIsAscending)
		{
			FVector CurrentLoc = GetActorLocation();
			FVector NewLoc = UKismetMathLibrary::VInterpTo_Constant(CurrentLoc, InitialLocation, DeltaTime, DescentSpeed);

			SetActorLocation(NewLoc, true);

			if (FVector::Dist(NewLoc, InitialLocation) < 10.0f)
			{
				bIsAscending = false;
			}
		}
	}
}

void AAbyssSubmarine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssSubmarine, bIsDescending);
	DOREPLIFETIME(AAbyssSubmarine, bIsAscending);
}