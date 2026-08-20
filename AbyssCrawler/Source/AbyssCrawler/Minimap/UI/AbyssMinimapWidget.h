#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Minimap/AbyssMinimapTypes.h"
#include "AbyssMinimapWidget.generated.h"

class AAbyssGameState;
class UAbyssMinimapIconWidget;
class UCanvasPanel;
class UPanelWidget;
struct FAbyssMinimapEntry;

USTRUCT()
struct FAbyssMinimapIconSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAbyssMinimapIconWidget> Widget = nullptr;

	// 10Hz 스냅샷 사이를 보간하기 위해 현재 화면에 그려지고 있는 위치(px)
	FVector2D DisplayPos = FVector2D::ZeroVector;

	bool bPlaced = false;

	// 아래는 "바뀐 프레임에만 BP를 호출"하기 위해 들고 있는 직전 값이다.
	// 닉네임 같은 값은 PlayerState보다 늦게 복제될 수 있어 생성 시 한 번만 읽으면 비어버린다.
	UPROPERTY()
	FText LastLabel;

	int32 LastColorIndex = INDEX_NONE;
	bool bLastClamped = false;
	bool bLastDead = false;
	bool bStateInitialized = false;
};

/**
 * 심해 미니맵 (XY 평면). WBP_MainHUD 안에 자식으로 배치되며 MissionUI와 동일하게
 * BindWidget으로 물리므로 별도 CreateWidget 코드가 필요 없다.
 *
 * 플레이어 중심 스크롤 · 북쪽(월드 +X) 고정. 시야 반경 밖 대상은 테두리에 클램프한다.
 */
UCLASS()
class ABYSSCRAWLER_API UAbyssMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 미니맵의 모든 시각 요소를 담는 컨테이너. 수중이 아닐 때 이쪽을 접는다.
	// UserWidget 루트를 접으면 NativeTick이 멈춰 스스로 다시 켤 수 없기 때문에,
	// 루트는 계속 살려두고 이 자식만 토글한다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> MinimapRoot;

	// 아이콘이 붙는 캔버스. 중심(앵커 0.5, 0.5)이 곧 로컬 플레이어 위치다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> IconCanvas;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	TMap<EAbyssMinimapIconType, TSubclassOf<UAbyssMinimapIconWidget>> IconWidgetClasses;

	// 미니맵이 담는 월드 반경 (uu)
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float ViewRadiusUU = 4000.f;

	// 미니맵 위젯의 반지름 (px). 실제 위젯 크기의 절반과 맞출 것
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float MinimapRadiusPx = 110.f;

	// 테두리에 클램프된 아이콘을 안쪽으로 들여놓는 여백 (px)
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float IconEdgeMarginPx = 10.f;

	// 10Hz 스냅샷 사이의 계단 현상을 없애는 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float IconInterpSpeed = 12.f;

	// true  = 내가 보는 방향이 항상 미니맵 위쪽 (맵 전체가 회전, 내 아이콘은 고정)
	// false = 북쪽(월드 +X) 고정 (맵은 안 돌고 내 아이콘만 회전)
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	bool bRotateWithCamera = true;

	// 배경 텍스처 팬을 BP가 처리하도록 넘기는 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_UpdateMapBackground(FVector2D WorldCenterXY, float ViewRadius);

private:
	void UpdateEntries(
		const TArray<FAbyssMinimapEntry>& Entries,
		const TCHAR* KeyPrefix,
		const FVector& SelfLocation,
		const APlayerState* SelfState,
		AAbyssGameState* GameState,
		float ViewYaw,
		float DeltaTime);

	// 미니맵 중앙에 고정되는 로컬 플레이어 아이콘. 복제 스냅샷이 아니라 로컬 폰에서 직접 구동한다.
	void UpdateLocalPlayerIcon(APlayerState* SelfState, float SelfIconYaw, AAbyssGameState* GameState);

	// 라벨 / 색상 / 클램프 / 사망 표시를 "값이 바뀐 프레임에만" BP로 밀어 넣는다
	void ApplyIconState(
		FAbyssMinimapIconSlot& IconSlot,
		const FAbyssMinimapEntry& Entry,
		bool bClamped,
		float DeltaZ,
		float IconYaw,
		AAbyssGameState* GameState);

	FAbyssMinimapIconSlot* CreateIconSlot(FName Key, const FAbyssMinimapEntry& Entry, bool bIsLocalPlayer);

	// 엔트리에 대응하는 표시용 라벨을 로컬에서 조회한다 (복제 데이터가 아니다)
	FText ResolveLabel(const FAbyssMinimapEntry& Entry, AAbyssGameState* GameState) const;

	void ClearIcons();

	// 아이콘 재사용 풀. 서버가 배열 순서를 바꿔도 아이콘이 뒤바뀌지 않도록 안정 키로 관리한다.
	UPROPERTY()
	TMap<FName, FAbyssMinimapIconSlot> IconSlots;

	// 이번 프레임에 살아있는 키. 매 틱 재사용해 할당을 피한다.
	TSet<FName> ActiveKeys;

	// IconWidgetClasses 설정 누락은 "아이콘이 아예 안 뜬다"로만 나타나 원인 찾기가 어렵다.
	// 타입별로 딱 한 번 경고 로그를 남기기 위한 기록.
	TSet<EAbyssMinimapIconType> ReportedMissingIconClasses;
};
