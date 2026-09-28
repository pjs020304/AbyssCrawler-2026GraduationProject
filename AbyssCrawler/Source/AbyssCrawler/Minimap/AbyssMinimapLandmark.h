#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssMinimapLandmark.generated.h"

class UBillboardComponent;

/**
 * 미니맵에 길찾기용 아이콘을 띄울 지점 (예: 심해의 파괴된 고층 건물 중앙).
 *
 * 레벨에 배치만 하면 UAbyssMinimapComponent가 서버에서 수집해 StaticEntries로 복제한다.
 * 액터 자체는 복제하지 않는다 (위치/라벨만 GameState 경유로 전달되므로 거리 제한이 없다).
 * 월드 파티션에서 멀리 떨어진 셀이 언로드돼도 수집되도록 공간 로딩을 끈다.
 */
UCLASS()
class ABYSSCRAWLER_API AAbyssMinimapLandmark : public AActor
{
	GENERATED_BODY()

public:
	AAbyssMinimapLandmark();

	const FText& GetLabel() const { return Label; }

protected:
	// 미니맵 아이콘 옆에 표시할 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	FText Label;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent;
#endif
};
