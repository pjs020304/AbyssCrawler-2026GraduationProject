#include "Minimap/AbyssMinimapComponent.h"

#include "AbyssGameState.h"
#include "AbyssSubmarine.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Mission/Contents/AbyssDataConsole.h"
#include "Mission/Contents/AbyssMissionArea.h"
#include "Mission/Contents/AbyssMissionItem.h"
#include "Mission/Contents/AbyssMissionWorkObject.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UAbyssMinimapComponent::UAbyssMinimapComponent()
{
	// 갱신은 서버 타이머로만 돈다. 컴포넌트 틱은 필요 없다.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAbyssMinimapComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		DynamicUpdateTimer, this, &UAbyssMinimapComponent::UpdateDynamicEntries,
		DynamicUpdateInterval, true);

	// 레벨의 미션 액터가 전부 BeginPlay를 마친 뒤 수집되도록 다음 틱으로 미룬다.
	RequestStaticRebuild();
}

void UAbyssMinimapComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbyssMinimapComponent, DynamicEntries);
	DOREPLIFETIME(UAbyssMinimapComponent, StaticEntries);
}

void UAbyssMinimapComponent::RequestStaticRebuild()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bStaticRebuildPending)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bStaticRebuildPending = true;

	// AAbyssMissionItem::Interact_Implementation은 미션 진행도를 먼저 올리고 그 뒤에 Destroy()를 부른다.
	// 여기서 즉시 재구축하면 방금 수집한 아이템이 미니맵에 그대로 남는다.
	// 다음 틱으로 미루면 파괴된 액터가 TActorIterator에서 이미 제외되고, 연속 호출도 한 번으로 합쳐진다.
	World->GetTimerManager().SetTimerForNextTick(this, &UAbyssMinimapComponent::RebuildStaticEntries);
}

void UAbyssMinimapComponent::UpdateDynamicEntries()
{
	AGameStateBase* GameState = Cast<AGameStateBase>(GetOwner());
	UWorld* World = GetWorld();
	if (!GameState || !World)
	{
		return;
	}

	// 배열을 통째로 다시 만든다. 엔진 FRepLayout이 섀도 상태와 프로퍼티 비교를 하므로
	// 값이 그대로면 아무것도 전송되지 않는다 (FVector_NetQuantize가 1uu로 반올림하니
	// 제자리에 서 있는 동안에는 자연히 억제된다).
	DynamicEntries.Reset();

	// PlayerArray 자체가 복제되고 PlayerState는 항상 릴리번트라, 클라는 이 참조를 늘 해석할 수 있다.
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		// 서버는 릴리번시와 무관하게 모든 폰의 위치를 안다
		APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		FAbyssMinimapEntry& Entry = DynamicEntries.AddDefaulted_GetRef();
		Entry.IconType = EAbyssMinimapIconType::Player;
		Entry.WorldLocation = Pawn->GetActorLocation();
		Entry.Yaw = Pawn->GetActorRotation().Yaw;
		Entry.OwnerState = PlayerState;
	}

	// 잠수함은 맵당 1개다. SetReplicateMovement(true)지만 릴리번시 컬링을 받으므로
	// 원거리에서도 보이도록 위치를 여기에 직접 실어 보낸다.
	if (!CachedSubmarine.IsValid())
	{
		for (TActorIterator<AAbyssSubmarine> It(World); It; ++It)
		{
			CachedSubmarine = *It;
			break;
		}
	}

	if (const AAbyssSubmarine* Submarine = CachedSubmarine.Get())
	{
		FAbyssMinimapEntry& Entry = DynamicEntries.AddDefaulted_GetRef();
		Entry.IconType = EAbyssMinimapIconType::Submarine;
		Entry.WorldLocation = Submarine->GetActorLocation();
	}
}

void UAbyssMinimapComponent::RebuildStaticEntries()
{
	bStaticRebuildPending = false;

	AAbyssGameState* GameState = Cast<AAbyssGameState>(GetOwner());
	UWorld* World = GetWorld();
	if (!GameState || !World)
	{
		return;
	}

	StaticEntries.Reset();

	// 수락했고 아직 완료되지 않은 미션만 목표를 노출한다
	TSet<FName> ActiveMissionIds;
	for (const FAbyssMissionData& Mission : GameState->Missions)
	{
		if (!Mission.bCompleted && !Mission.MissionId.IsNone())
		{
			ActiveMissionIds.Add(Mission.MissionId);
		}
	}

	if (ActiveMissionIds.Num() == 0)
	{
		return;
	}

	auto AddObjective = [this, &ActiveMissionIds](FName MissionId, const FVector& Location)
	{
		if (!ActiveMissionIds.Contains(MissionId))
		{
			return;
		}

		FAbyssMinimapEntry& Entry = StaticEntries.AddDefaulted_GetRef();
		Entry.IconType = EAbyssMinimapIconType::MissionObjective;
		Entry.WorldLocation = Location;
		Entry.MissionId = MissionId;
	};

	// MissionId로 월드 액터를 훑는 방식은 AbyssGameMode의 기존 관례를 그대로 따른다
	for (TActorIterator<AAbyssMissionItem> It(World); It; ++It)
	{
		AddObjective(It->MissionId, It->GetActorLocation());
	}
	for (TActorIterator<AAbyssMissionArea> It(World); It; ++It)
	{
		AddObjective(It->GetMissionId(), It->GetActorLocation());
	}
	for (TActorIterator<AAbyssMissionWorkObject> It(World); It; ++It)
	{
		AddObjective(It->GetMissionId(), It->GetActorLocation());
	}
	for (TActorIterator<AAbyssDataConsole> It(World); It; ++It)
	{
		AddObjective(It->GetMissionId(), It->GetActorLocation());
	}
}
