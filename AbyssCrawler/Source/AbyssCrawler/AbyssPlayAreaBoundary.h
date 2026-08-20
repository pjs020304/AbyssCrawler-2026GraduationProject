#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssPlayAreaBoundary.generated.h"

class UBoxComponent;

/**
 * 심해 플레이 영역 경계 액터.
 * 박스 형태의 플레이 영역을 정의하고, 6면에 보이지 않는 벽(Pawn 전용 차단)을 자동 생성한다.
 * 로컬 플레이어가 경계에 가까워지면 HUD에 경고를 표시한다.
 * 맵당 1개 배치. 크기는 액터 스케일이 아닌 PlayAreaExtent로 조절할 것.
 */
UCLASS()
class ABYSSCRAWLER_API AAbyssPlayAreaBoundary : public AActor
{
	GENERATED_BODY()

public:
	AAbyssPlayAreaBoundary();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

	// 월드 좌표에서 가장 가까운 벽 안쪽 면까지의 거리 (영역 밖이면 음수)
	UFUNCTION(BlueprintPure, Category = "Boundary")
	float GetDistanceToNearestWall(const FVector& WorldLocation) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	USceneComponent* SceneRoot;

	// 에디터에서 영역을 와이어프레임으로 보여주는 마커 (충돌 없음)
	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	UBoxComponent* PlayAreaVisual;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	UBoxComponent* WallXPos;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	UBoxComponent* WallXNeg;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	UBoxComponent* WallYPos;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	UBoxComponent* WallYNeg;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	UBoxComponent* WallZPos;

	UPROPERTY(VisibleAnywhere, Category = "Boundary")
	UBoxComponent* WallZNeg;

	// 플레이 영역 절반 크기 (중심 기준 Half-size)
	UPROPERTY(EditAnywhere, Category = "Boundary")
	FVector PlayAreaExtent = FVector(10000.f, 10000.f, 5000.f);

	// 벽 두께 (Half-size). 고속 이동 시 터널링 방지를 위해 충분히 두껍게 유지
	UPROPERTY(EditAnywhere, Category = "Boundary")
	float WallThickness = 100.f;

	// 벽에서 이 거리 안으로 들어오면 HUD 경고 표시
	UPROPERTY(EditAnywhere, Category = "Boundary|Warning")
	float WarningDistance = 1500.f;

	// 경고 체크 주기 (초)
	UPROPERTY(EditAnywhere, Category = "Boundary|Warning")
	float WarningCheckInterval = 0.2f;

private:
	bool bWarningShown = false;
	float TimeSinceLastCheck = 0.f;

	TArray<UBoxComponent*> GetWalls() const;
	void UpdateWallLayout();
	void UpdateWarningForLocalPlayer();
};
