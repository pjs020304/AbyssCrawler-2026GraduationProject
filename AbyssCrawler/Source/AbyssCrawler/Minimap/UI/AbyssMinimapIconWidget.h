#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Minimap/AbyssMinimapTypes.h"
#include "AbyssMinimapIconWidget.generated.h"

/**
 * 미니맵 아이콘 한 개. 배치와 좌표 계산은 UAbyssMinimapWidget이 전부 처리하고,
 * 이 클래스는 시각 표현을 BP에 넘기는 훅만 갖는다 (MainHUDWidget의 BP_* 관례와 동일).
 */
UCLASS()
class ABYSSCRAWLER_API UAbyssMinimapIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 아이콘이 처음 만들어질 때 한 번만 호출된다.
	// bIsLocalPlayer는 미니맵 중앙에 고정되는 "나" 아이콘을 팀원과 다르게 그리기 위한 구분값이다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_InitIcon(EAbyssMinimapIconType IconType, bool bIsLocalPlayer);

	// 플레이어 닉네임 또는 미션 이름. PlayerState보다 Nickname이 늦게 복제될 수 있어
	// 매 틱 다시 계산하되, 실제로 값이 바뀐 프레임에만 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_SetLabel(const FText& Label);

	// 로비에서 배정된 플레이어 색 인덱스. 라벨과 같은 이유로 변경 시에만 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_SetPlayerColorIndex(int32 PlayerColorIndex);

	// 시야 반경 밖이라 미니맵 테두리에 붙었는지 여부. BP에서 화살표 모양으로 바꾸는 용도
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_SetClamped(bool bClamped);

	// 로컬 플레이어 기준 상대 높이(uu). 양수면 나보다 위에 있다.
	// 미니맵이 XY만 보여주므로 깊이 차이는 이 훅으로 표기한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_SetRelativeDepth(float DeltaZ);

	// 월드 Yaw(도). 월드 +X(북)가 미니맵 위쪽이고 Slate의 양수 회전이 시계 방향이라
	// 북쪽 고정 미니맵에서는 이 값을 그대로 회전에 써도 된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_SetIconYaw(float Yaw);

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void BP_SetDead(bool bDead);
};
