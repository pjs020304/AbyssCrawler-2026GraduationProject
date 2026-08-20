#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "AbyssMinimapTypes.generated.h"

class APlayerState;

UENUM(BlueprintType)
enum class EAbyssMinimapIconType : uint8
{
	Player            UMETA(DisplayName = "Player"),           // 팀원
	Submarine         UMETA(DisplayName = "Submarine"),        // 잠수함 (귀환 지점)
	MissionObjective  UMETA(DisplayName = "MissionObjective")  // 활성 미션 목표
};

/**
 * 미니맵 표시 전용 위치 스냅샷 엔트리.
 *
 * 플레이어 폰과 미션 목표 액터는 네트워크 릴리번시 컬링(기본 15,000uu)을 받는데
 * 플레이 영역은 XY 대각선으로 약 28,000uu다. 즉 클라이언트는 멀리 있는 팀원의 위치를
 * 아예 모른다. 그래서 위치를 "표시용 데이터"로 따로 떼어내 항상 릴리번트한
 * GameState(UAbyssMinimapComponent)에 실어 보낸다.
 *
 * BlueprintType이 아닌 이유: FVector_NetQuantize가 BlueprintType이 아니라 BP 노출이 불가능하다.
 * 위젯을 C++로 두고 BP에는 가공된 값만 넘기는 프로젝트 관례(MainHUDWidget의 BP_* 패턴)와도 맞다.
 */
USTRUCT()
struct FAbyssMinimapEntry
{
	GENERATED_BODY()

	UPROPERTY()
	EAbyssMinimapIconType IconType = EAbyssMinimapIconType::Player;

	// 미니맵은 XY만 쓰지만 Z는 "나보다 위/아래" 표기에 사용한다.
	// SerializePackedVector<1,20> = 1uu 정밀도, ±1,048,576uu 범위 (플레이 영역 ±10,000uu에 충분)
	UPROPERTY()
	FVector_NetQuantize WorldLocation = FVector::ZeroVector;

	// 아이콘 회전용 월드 Yaw(도). Player 타입에서만 유효
	UPROPERTY()
	float Yaw = 0.f;

	// Player 타입에서만 유효. 클라가 닉네임/색상을 로컬 조회하는 키이자 아이콘 풀링 키.
	// PlayerState는 bAlwaysRelevant이므로 이 참조는 모든 클라에서 항상 해석된다.
	UPROPERTY()
	TObjectPtr<APlayerState> OwnerState = nullptr;

	// MissionObjective 타입에서만 유효. 라벨 표시 및 아이콘 풀링 키
	UPROPERTY()
	FName MissionId = NAME_None;
};
