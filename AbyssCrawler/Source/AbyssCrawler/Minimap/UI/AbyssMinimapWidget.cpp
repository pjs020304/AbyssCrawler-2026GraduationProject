#include "Minimap/UI/AbyssMinimapWidget.h"

#include "AbyssDiverCharacter.h"
#include "AbyssGameState.h"
#include "AbyssPlayerState.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Minimap/AbyssMinimapComponent.h"
#include "Minimap/UI/AbyssMinimapIconWidget.h"

namespace
{
	const FName LocalPlayerIconKey(TEXT("__LocalPlayer"));

	// 아이콘 풀의 안정 키를 만든다.
	// 플레이어는 PlayerState 이름으로 잡아 PlayerArray 순서가 바뀌어도 아이콘이 유지되게 하고,
	// 미션 목표는 같은 MissionId를 가진 액터가 여럿(BP_MissionItem 21개)이라 인덱스까지 넣어야 유일해진다.
	FName MakeIconKey(const FAbyssMinimapEntry& Entry, const TCHAR* KeyPrefix, int32 Index)
	{
		switch (Entry.IconType)
		{
		case EAbyssMinimapIconType::Player:
			// PlayerState 참조가 아직 해석되지 않았어도 위치 자체는 유효하다.
			// 여기서 포기하면 팀원 아이콘이 통째로 사라지므로 인덱스 키로 폴백한다.
			// 라벨은 매 틱 다시 조회하므로 참조가 붙는 순간 자동으로 채워진다.
			return Entry.OwnerState
				? Entry.OwnerState->GetFName()
				: FName(*FString::Printf(TEXT("%s_Player_%d"), KeyPrefix, Index));

		case EAbyssMinimapIconType::Submarine:
			return FName(TEXT("__Submarine"));

		default:
			// MissionId를 키에 넣지 않으면, 미션 하나가 완료돼 배열이 앞으로 밀렸을 때
			// 기존 아이콘이 다른 미션의 목표로 재사용된다.
			return FName(*FString::Printf(TEXT("%s_%s_%d"), KeyPrefix, *Entry.MissionId.ToString(), Index));
		}
	}
}

void UAbyssMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 루트 위젯이 Collapsed/Hidden이면 Slate가 NativeTick을 부르지 않아 스스로 다시 켤 수 없다.
	// 그래서 루트는 항상 살려두고(디자이너 설정과 무관하게 강제) 실제 표시는 MinimapRoot로만 토글한다.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (MinimapRoot)
	{
		MinimapRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAbyssMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AAbyssDiverCharacter* Diver = Cast<AAbyssDiverCharacter>(GetOwningPlayerPawn());
	AAbyssGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AAbyssGameState>() : nullptr;

	const bool bShouldShow =
		Diver && Diver->IsInWater() &&
		GameState && GameState->GetGamePhase() == EAbyssGamePhase::Playing;

	if (!bShouldShow)
	{
		if (MinimapRoot && MinimapRoot->GetVisibility() != ESlateVisibility::Collapsed)
		{
			MinimapRoot->SetVisibility(ESlateVisibility::Collapsed);
			ClearIcons();
		}
		return;
	}

	if (MinimapRoot && MinimapRoot->GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
	{
		MinimapRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UAbyssMinimapComponent* Minimap = GameState->GetMinimapComponent();
	if (!Minimap || !IconCanvas)
	{
		return;
	}

	// 내 위치와 시선은 복제 스냅샷을 거치지 않고 로컬에서 직접 읽는다.
	// 지연이 0이어야 하고, 이 값이 미니맵의 원점이자 나머지 모든 좌표의 기준이기 때문이다.
	const FVector SelfLocation = Diver->GetActorLocation();
	APlayerState* SelfState = GetOwningPlayerState();

	const APlayerController* OwningPC = GetOwningPlayer();
	const float SelfYaw = OwningPC ? OwningPC->GetControlRotation().Yaw : Diver->GetActorRotation().Yaw;

	// 회전 모드에서는 시선 방향이 위쪽으로 오도록 맵 전체를 -SelfYaw만큼 돌린다.
	// 북쪽 고정 모드에서는 맵을 돌리지 않는 대신 내 아이콘이 실제 방향으로 회전한다.
	const float ViewYaw = bRotateWithCamera ? SelfYaw : 0.f;
	const float SelfIconYaw = SelfYaw - ViewYaw;

	ActiveKeys.Reset();

	UpdateLocalPlayerIcon(SelfState, SelfIconYaw, GameState);
	UpdateEntries(Minimap->GetDynamicEntries(), TEXT("Dyn"), SelfLocation, SelfState, GameState, ViewYaw, InDeltaTime);
	if (bShowMissionObjectives)
	{
		// 끈 상태로 두면 이 키들이 ActiveKeys에 안 들어가므로, 아래 정리 루프가 아이콘을 걷어낸다.
		UpdateEntries(Minimap->GetStaticEntries(), TEXT("Obj"), SelfLocation, SelfState, GameState, ViewYaw, InDeltaTime);
	}

	// 이번 프레임에 사라진 엔트리의 아이콘을 정리한다
	for (TMap<FName, FAbyssMinimapIconSlot>::TIterator It(IconSlots); It; ++It)
	{
		if (ActiveKeys.Contains(It.Key()))
		{
			continue;
		}

		if (It.Value().Widget)
		{
			It.Value().Widget->RemoveFromParent();
		}
		It.RemoveCurrent();
	}

	BP_UpdateMapBackground(FVector2D(SelfLocation.X, SelfLocation.Y), ViewRadiusUU);

	if (bLogIconDiagnostics)
	{
		DiagnosticsAccumulator += InDeltaTime;
		if (DiagnosticsAccumulator >= 1.f)
		{
			DiagnosticsAccumulator = 0.f;
			LogDiagnostics(*Minimap, SelfLocation);
		}
	}
}

void UAbyssMinimapWidget::UpdateLocalPlayerIcon(APlayerState* SelfState, float SelfIconYaw, AAbyssGameState* GameState)
{
	FAbyssMinimapEntry LocalEntry;
	LocalEntry.IconType = EAbyssMinimapIconType::Player;
	LocalEntry.OwnerState = SelfState;

	FAbyssMinimapIconSlot* IconSlot = IconSlots.Find(LocalPlayerIconKey);
	if (!IconSlot)
	{
		IconSlot = CreateIconSlot(LocalPlayerIconKey, LocalEntry, /*bIsLocalPlayer=*/true);
		if (!IconSlot)
		{
			return;
		}
	}
	ActiveKeys.Add(LocalPlayerIconKey);

	// 나는 언제나 미니맵의 원점이다
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IconSlot->Widget->Slot))
	{
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
	}
	IconSlot->DisplayPos = FVector2D::ZeroVector;
	IconSlot->bPlaced = true;

	ApplyIconState(*IconSlot, LocalEntry, /*bClamped=*/false, /*DeltaZ=*/0.f, SelfIconYaw, GameState);
}

void UAbyssMinimapWidget::UpdateEntries(
	const TArray<FAbyssMinimapEntry>& Entries,
	const TCHAR* KeyPrefix,
	const FVector& SelfLocation,
	const APlayerState* SelfState,
	AAbyssGameState* GameState,
	float ViewYaw,
	float DeltaTime)
{
	// 0으로 나누는 사고를 막는다 (에디터에서 실수로 0을 넣을 수 있다)
	const float SafeViewRadius = FMath::Max(ViewRadiusUU, 1.f);

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FAbyssMinimapEntry& Entry = Entries[Index];

		// 내 아이콘은 UpdateLocalPlayerIcon이 로컬 값으로 따로 그린다
		if (Entry.IconType == EAbyssMinimapIconType::Player && SelfState && Entry.OwnerState.Get() == SelfState)
		{
			continue;
		}

		const FName Key = MakeIconKey(Entry, KeyPrefix, Index);
		if (Key.IsNone())
		{
			continue;
		}

		FAbyssMinimapIconSlot* IconSlot = IconSlots.Find(Key);
		if (!IconSlot)
		{
			IconSlot = CreateIconSlot(Key, Entry, /*bIsLocalPlayer=*/false);
			if (!IconSlot)
			{
				continue;
			}
		}
		ActiveKeys.Add(Key);

		// 언리얼 월드는 +X = 북, +Y = 동. Slate 화면은 +X = 오른쪽, +Y = 아래라 두 축이 서로 교차한다.
		FVector2D Delta(
			(Entry.WorldLocation.X - SelfLocation.X) / SafeViewRadius,   // 북쪽 성분
			(Entry.WorldLocation.Y - SelfLocation.Y) / SafeViewRadius);  // 동쪽 성분

		if (!FMath::IsNearlyZero(ViewYaw))
		{
			// 시선 방향이 위쪽으로 오도록 (북, 동) 평면을 -ViewYaw만큼 회전시킨다
			const float Rad = FMath::DegreesToRadians(-ViewYaw);
			const float SinYaw = FMath::Sin(Rad);
			const float CosYaw = FMath::Cos(Rad);
			Delta = FVector2D(
				Delta.X * CosYaw - Delta.Y * SinYaw,
				Delta.X * SinYaw + Delta.Y * CosYaw);
		}

		// 원형 미니맵: 시야 반경 밖은 테두리로 투영한다
		const float Length = Delta.Size();
		const bool bClamped = Length > 1.f;
		const FVector2D Unit = bClamped ? Delta / Length : Delta;

		// 클램프 여부와 무관하게 같은 반경을 쓴다. 다르게 주면 시야 경계를 넘는 순간 아이콘이 툭 튄다.
		const float Radius = FMath::Max(MinimapRadiusPx - IconEdgeMarginPx, 0.f);
		const FVector2D TargetPos(
			Unit.Y * Radius,    // 동 → 화면 오른쪽
			-Unit.X * Radius);  // 북 → 화면 위 (Slate의 Y는 아래로 증가)

		IconSlot->DisplayPos = IconSlot->bPlaced
			? FMath::Vector2DInterpTo(IconSlot->DisplayPos, TargetPos, DeltaTime, IconInterpSpeed)
			: TargetPos;
		IconSlot->bPlaced = true;

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IconSlot->Widget->Slot))
		{
			CanvasSlot->SetPosition(IconSlot->DisplayPos);
		}

		// 팀원 아이콘의 방향도 맵 회전량만큼 함께 보정한다
		const float IconYaw = (Entry.IconType == EAbyssMinimapIconType::Player) ? Entry.Yaw - ViewYaw : 0.f;

		ApplyIconState(*IconSlot, Entry, bClamped, Entry.WorldLocation.Z - SelfLocation.Z, IconYaw, GameState);
	}
}

void UAbyssMinimapWidget::ApplyIconState(
	FAbyssMinimapIconSlot& IconSlot,
	const FAbyssMinimapEntry& Entry,
	bool bClamped,
	float DeltaZ,
	float IconYaw,
	AAbyssGameState* GameState)
{
	// 아이콘이 살아 있는 한 루트는 항상 보이도록 C++이 못 박는다.
	// BP의 BP_SetClamped/BP_SetDead 구현이 한쪽 분기만 채워져 있어도 아이콘이 통째로
	// 사라지지 않게 하기 위한 안전장치다.
	if (IconSlot.Widget->GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
	{
		IconSlot.Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 연속 값은 매 프레임 갱신한다
	IconSlot.Widget->BP_SetRelativeDepth(DeltaZ);
	if (Entry.IconType == EAbyssMinimapIconType::Player)
	{
		IconSlot.Widget->BP_SetIconYaw(IconYaw);
	}

	// 나머지는 실제로 바뀐 프레임에만 BP를 호출한다.
	// 매 틱 다시 계산하는 이유는, 닉네임/색상이 PlayerState보다 늦게 복제될 수 있어
	// 생성 시 한 번만 읽으면 빈 라벨로 굳어버리기 때문이다.
	const AAbyssPlayerState* AbyssState = Cast<AAbyssPlayerState>(Entry.OwnerState.Get());
	const FText Label = ResolveLabel(Entry, GameState);
	const int32 ColorIndex = AbyssState ? AbyssState->PlayerColorIndex : 0;
	const bool bDead = AbyssState && !AbyssState->bIsAlive;

	if (!IconSlot.bStateInitialized || !IconSlot.LastLabel.EqualTo(Label))
	{
		IconSlot.LastLabel = Label;
		IconSlot.Widget->BP_SetLabel(Label);
	}
	if (IconSlot.LastColorIndex != ColorIndex)
	{
		IconSlot.LastColorIndex = ColorIndex;
		IconSlot.Widget->BP_SetPlayerColorIndex(ColorIndex);
	}
	if (!IconSlot.bStateInitialized || IconSlot.bLastClamped != bClamped)
	{
		IconSlot.bLastClamped = bClamped;
		IconSlot.Widget->BP_SetClamped(bClamped);
	}
	if (!IconSlot.bStateInitialized || IconSlot.bLastDead != bDead)
	{
		IconSlot.bLastDead = bDead;
		IconSlot.Widget->BP_SetDead(bDead);
	}

	IconSlot.bStateInitialized = true;
}

FText UAbyssMinimapWidget::ResolveLabel(const FAbyssMinimapEntry& Entry, AAbyssGameState* GameState) const
{
	// PlayerState는 항상 릴리번트라 닉네임은 언제든 로컬에서 읽을 수 있다.
	// FText를 매 스냅샷마다 네트워크로 실어 보낼 이유가 없다.
	if (const AAbyssPlayerState* AbyssState = Cast<AAbyssPlayerState>(Entry.OwnerState.Get()))
	{
		return AbyssState->Nickname;
	}

	if (Entry.IconType == EAbyssMinimapIconType::MissionObjective && GameState)
	{
		for (const FAbyssMissionData& Mission : GameState->Missions)
		{
			if (Mission.MissionId == Entry.MissionId)
			{
				return Mission.MissionTitle;
			}
		}
	}

	return FText::GetEmpty();
}

FAbyssMinimapIconSlot* UAbyssMinimapWidget::CreateIconSlot(FName Key, const FAbyssMinimapEntry& Entry, bool bIsLocalPlayer)
{
	if (!IconCanvas)
	{
		return nullptr;
	}

	const TSubclassOf<UAbyssMinimapIconWidget> IconClass = GetIconClass(Entry.IconType);
	if (!IconClass)
	{
		// 설정 누락은 "아이콘이 아예 안 뜬다"로만 드러나 원인을 찾기 어렵다. 타입별로 한 번만 알린다.
		if (!ReportedMissingIconClasses.Contains(Entry.IconType))
		{
			ReportedMissingIconClasses.Add(Entry.IconType);
			UE_LOG(LogTemp, Warning,
				TEXT("[Minimap] %s 아이콘 클래스가 비어 있어 아이콘을 만들 수 없습니다. WBP_Minimap의 Class Defaults에서 채우세요."),
				*UEnum::GetValueAsString(Entry.IconType));
		}
		return nullptr;
	}

	UAbyssMinimapIconWidget* IconWidget = CreateWidget<UAbyssMinimapIconWidget>(GetOwningPlayer(), IconClass);
	if (!IconWidget)
	{
		return nullptr;
	}

	UCanvasPanelSlot* CanvasSlot = IconCanvas->AddChildToCanvas(IconWidget);
	if (CanvasSlot)
	{
		// 캔버스 중심을 로컬 플레이어 위치(원점)로 쓰기 위해 앵커/정렬을 C++에서 고정한다.
		// WBP 쪽 설정 실수로 좌표계가 어긋나는 걸 막는다.
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(true);
	}

	IconWidget->BP_InitIcon(Entry.IconType, bIsLocalPlayer);

	FAbyssMinimapIconSlot& NewSlot = IconSlots.Add(Key);
	NewSlot.Widget = IconWidget;
	return &NewSlot;
}

TSubclassOf<UAbyssMinimapIconWidget> UAbyssMinimapWidget::GetIconClass(EAbyssMinimapIconType IconType) const
{
	switch (IconType)
	{
	case EAbyssMinimapIconType::Player:           return PlayerIconClass;
	case EAbyssMinimapIconType::Submarine:        return SubmarineIconClass;
	case EAbyssMinimapIconType::MissionObjective: return MissionObjectiveIconClass;
	default:                                      return nullptr;
	}
}

void UAbyssMinimapWidget::LogDiagnostics(const UAbyssMinimapComponent& Minimap, const FVector& SelfLocation) const
{
	UE_LOG(LogTemp, Warning,
		TEXT("[Minimap] Dynamic=%d Static=%d Icons=%d Self=(%.0f, %.0f) ViewRadius=%.0f RadiusPx=%.0f"),
		Minimap.GetDynamicEntries().Num(), Minimap.GetStaticEntries().Num(), IconSlots.Num(),
		SelfLocation.X, SelfLocation.Y, ViewRadiusUU, MinimapRadiusPx);

	for (const TPair<FName, FAbyssMinimapIconSlot>& Pair : IconSlots)
	{
		const UAbyssMinimapIconWidget* Icon = Pair.Value.Widget;
		UE_LOG(LogTemp, Warning,
			TEXT("[Minimap]   %-24s pos=(%7.1f,%7.1f) clamped=%d visible=%s"),
			*Pair.Key.ToString(),
			Pair.Value.DisplayPos.X, Pair.Value.DisplayPos.Y,
			Pair.Value.bLastClamped ? 1 : 0,
			Icon ? *UEnum::GetValueAsString(Icon->GetVisibility()) : TEXT("<null>"));
	}
}

void UAbyssMinimapWidget::ClearIcons()
{
	for (TPair<FName, FAbyssMinimapIconSlot>& Pair : IconSlots)
	{
		if (Pair.Value.Widget)
		{
			Pair.Value.Widget->RemoveFromParent();
		}
	}

	IconSlots.Reset();
}
