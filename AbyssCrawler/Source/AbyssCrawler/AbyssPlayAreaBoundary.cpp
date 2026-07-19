#include "AbyssPlayAreaBoundary.h"

#include "AbyssDiverCharacter.h"
#include "Components/BoxComponent.h"
#include "MainHUDWidget.h"

AAbyssPlayAreaBoundary::AAbyssPlayAreaBoundary()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PlayAreaVisual = CreateDefaultSubobject<UBoxComponent>(TEXT("PlayAreaVisual"));
	PlayAreaVisual->SetupAttachment(RootComponent);
	PlayAreaVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlayAreaVisual->SetHiddenInGame(true);

	WallXPos = CreateDefaultSubobject<UBoxComponent>(TEXT("WallXPos"));
	WallXNeg = CreateDefaultSubobject<UBoxComponent>(TEXT("WallXNeg"));
	WallYPos = CreateDefaultSubobject<UBoxComponent>(TEXT("WallYPos"));
	WallYNeg = CreateDefaultSubobject<UBoxComponent>(TEXT("WallYNeg"));
	WallZPos = CreateDefaultSubobject<UBoxComponent>(TEXT("WallZPos"));
	WallZNeg = CreateDefaultSubobject<UBoxComponent>(TEXT("WallZNeg"));

	for (UBoxComponent* Wall : GetWalls())
	{
		Wall->SetupAttachment(RootComponent);
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetCollisionObjectType(ECC_WorldStatic);
		// Pawn만 차단해서 라이트/상호작용 트레이스, 투사체 등에는 영향을 주지 않는다
		Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
		Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Wall->SetHiddenInGame(true);
	}

	UpdateWallLayout();
}

void AAbyssPlayAreaBoundary::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateWallLayout();
}

TArray<UBoxComponent*> AAbyssPlayAreaBoundary::GetWalls() const
{
	return { WallXPos, WallXNeg, WallYPos, WallYNeg, WallZPos, WallZNeg };
}

void AAbyssPlayAreaBoundary::UpdateWallLayout()
{
	const FVector E = PlayAreaExtent;
	const float T = WallThickness;

	PlayAreaVisual->SetBoxExtent(E);

	// 벽을 모서리에서 서로 겹치게 배치해 대각선 방향으로도 틈이 생기지 않게 한다
	WallXPos->SetRelativeLocation(FVector(E.X + T, 0.f, 0.f));
	WallXPos->SetBoxExtent(FVector(T, E.Y + 2.f * T, E.Z + 2.f * T));

	WallXNeg->SetRelativeLocation(FVector(-(E.X + T), 0.f, 0.f));
	WallXNeg->SetBoxExtent(FVector(T, E.Y + 2.f * T, E.Z + 2.f * T));

	WallYPos->SetRelativeLocation(FVector(0.f, E.Y + T, 0.f));
	WallYPos->SetBoxExtent(FVector(E.X + 2.f * T, T, E.Z + 2.f * T));

	WallYNeg->SetRelativeLocation(FVector(0.f, -(E.Y + T), 0.f));
	WallYNeg->SetBoxExtent(FVector(E.X + 2.f * T, T, E.Z + 2.f * T));

	WallZPos->SetRelativeLocation(FVector(0.f, 0.f, E.Z + T));
	WallZPos->SetBoxExtent(FVector(E.X + 2.f * T, E.Y + 2.f * T, T));

	WallZNeg->SetRelativeLocation(FVector(0.f, 0.f, -(E.Z + T)));
	WallZNeg->SetBoxExtent(FVector(E.X + 2.f * T, E.Y + 2.f * T, T));
}

float AAbyssPlayAreaBoundary::GetDistanceToNearestWall(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	return FMath::Min3(
		PlayAreaExtent.X - FMath::Abs(Local.X),
		PlayAreaExtent.Y - FMath::Abs(Local.Y),
		PlayAreaExtent.Z - FMath::Abs(Local.Z));
}

void AAbyssPlayAreaBoundary::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastCheck += DeltaTime;
	if (TimeSinceLastCheck < WarningCheckInterval)
	{
		return;
	}
	TimeSinceLastCheck = 0.f;

	UpdateWarningForLocalPlayer();
}

void AAbyssPlayAreaBoundary::UpdateWarningForLocalPlayer()
{
	// 경고 UI는 로컬 전용 연출 — 데디케이티드 서버나 로컬 폰이 없으면 아무것도 하지 않는다
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(PC->GetPawn());
	if (!Diver || !Diver->IsLocallyControlled() || !Diver->MainHUDRef)
	{
		return;
	}

	const float Dist = GetDistanceToNearestWall(Diver->GetActorLocation());
	if (Dist < WarningDistance)
	{
		const float Intensity = 1.f - FMath::Clamp(Dist / WarningDistance, 0.f, 1.f);
		Diver->MainHUDRef->BP_ShowBoundaryWarning(Intensity);
		bWarningShown = true;
	}
	else if (bWarningShown)
	{
		Diver->MainHUDRef->BP_HideBoundaryWarning();
		bWarningShown = false;
	}
}
