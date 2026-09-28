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
#include "AbyssItemBase.h"
#include "Engine/OverlapResult.h"

AAbyssSubmarine::AAbyssSubmarine()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	// 위치는 CurrentMove로 모든 머신이 직접 계산한다 (ReplicateMovement를 켜면 계단식 보정이 섞여 다시 떨린다)
	SetReplicateMovement(false);

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

	// BP에 예전 기본값(ReplicateMovement=true)이 저장돼 있어도 확실히 끈다
	if (HasAuthority())
	{
		SetReplicateMovement(false);
	}
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
	BeginMove(TargetLocation);
}

void AAbyssSubmarine::StartAscent()
{
	bIsAscending = true;
	bIsDescending = false;
	BeginMove(InitialLocation);
}

double AAbyssSubmarine::GetSyncedWorldTime() const
{
	if (const AGameStateBase* GS = GetWorld()->GetGameState())
	{
		return GS->GetServerWorldTimeSeconds();
	}
	return GetWorld()->GetTimeSeconds();
}

void AAbyssSubmarine::BeginMove(const FVector& EndLocation)
{
	if (!HasAuthority()) return;

	const FVector StartLocation = GetActorLocation();
	const float Speed = FMath::Max(DescentSpeed, 1.0f);

	CurrentMove.StartLocation = StartLocation;
	CurrentMove.EndLocation = EndLocation;
	CurrentMove.StartServerTime = GetSyncedWorldTime();
	CurrentMove.Duration = FVector::Dist(StartLocation, EndLocation) / Speed;
	++CurrentMove.MoveId;

	ReleaseCargo();
	StowCargo();
	ForceNetUpdate();
}

void AAbyssSubmarine::OnRep_CurrentMove()
{
	// 클라이언트: 새 이동 구간 수신 → Tick에서 서버 시간에 맞춰 재생
	bLocalMoveActive = true;
	ApplyMoveAtCurrentTime();
}

bool AAbyssSubmarine::ApplyMoveAtCurrentTime()
{
	const float Alpha = (CurrentMove.Duration > KINDA_SMALL_NUMBER)
		? FMath::Clamp(static_cast<float>((GetSyncedWorldTime() - CurrentMove.StartServerTime) / CurrentMove.Duration), 0.0f, 1.0f)
		: 1.0f;

	// 스윕 없이 이동: 루트가 충돌 없는 SceneComponent라 스윕은 원래 의미가 없고,
	// 선체 메시는 키네마틱으로 움직여 위의 캐릭터(베이스 이동)/물체를 밀어준다.
	SetActorLocation(FMath::Lerp(CurrentMove.StartLocation, CurrentMove.EndLocation, Alpha));

	return Alpha >= 1.0f;
}

bool AAbyssSubmarine::IsInsideInterior(const FVector& WorldLocation) const
{
	if (!InteriorVolume) return false;

	const FVector Local = InteriorVolume->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = InteriorVolume->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool AAbyssSubmarine::TryStowItem(AAbyssItemBase* Item)
{
	if (!HasAuthority() || !(bIsDescending || bIsAscending)) return false;
	if (!IsValid(Item) || Item->IsPickedUp() || Item->IsStowed() || Item->GetAttachParentActor() != nullptr) return false;
	if (!IsInsideInterior(Item->GetActorLocation())) return false;

	// 선체 메시는 비균등 스케일(0.5, 0.6, 0.5)이라 여기에 붙이면 상대 변환이 왜곡된다.
	// 스케일 1인 루트에 붙인다 (루트와 메시는 함께 움직인다).
	Item->SetStowedIn(GetRootComponent());
	StowedCargo.AddUnique(Item);
	return true;
}

void AAbyssSubmarine::StowCargo()
{
	UWorld* World = GetWorld();
	if (!World || !InteriorVolume) return;

	// InteriorVolume은 Pawn만 겹치도록 되어 있어 아이템은 직접 쿼리로 찾는다
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SubmarineCargo), false, this);
	World->OverlapMultiByObjectType(
		Overlaps,
		InteriorVolume->GetComponentLocation(),
		InteriorVolume->GetComponentQuat(),
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeBox(InteriorVolume->GetScaledBoxExtent()),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAbyssItemBase* Item = Cast<AAbyssItemBase>(Overlap.GetActor());
		if (!IsValid(Item) || Item->IsPickedUp() || Item->IsStowed() || Item->GetAttachParentActor() != nullptr)
		{
			continue;
		}

		Item->SetStowedIn(GetRootComponent());
		StowedCargo.AddUnique(Item);
	}
}

void AAbyssSubmarine::ReleaseCargo()
{
	for (AAbyssItemBase* Item : StowedCargo)
	{
		// 이동 중 누가 주워 갔으면 이미 고정이 풀려 있다
		if (IsValid(Item) && Item->IsStowed())
		{
			Item->SetStowedIn(nullptr);
		}
	}
	StowedCargo.Reset();
}

void AAbyssSubmarine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (bIsDescending || bIsAscending)
		{
			if (ApplyMoveAtCurrentTime())
			{
				bIsDescending = false;
				bIsAscending = false;
				ReleaseCargo();
			}
		}
	}
	else if (bLocalMoveActive)
	{
		// 클라이언트도 서버와 같은 시간축으로 직접 이동 → 매 프레임 연속적인 위치
		if (ApplyMoveAtCurrentTime())
		{
			bLocalMoveActive = false;
		}
	}
}

void AAbyssSubmarine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAbyssSubmarine, bIsDescending);
	DOREPLIFETIME(AAbyssSubmarine, bIsAscending);
	DOREPLIFETIME(AAbyssSubmarine, CurrentMove);
}