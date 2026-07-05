// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssPCGManager.generated.h"

class ASVOVolume;
class UPCGComponent;

/**
 * 심해 환경 PCG(산호/암초/아이템 등)를 조율하는 매니저.
 *
 * 핵심 책임 2가지:
 *  1) [결정론 동기화] 서버가 랜덤 시드를 정해 리플리케이트 → 모든 클라이언트가
 *     동일 시드로 PCG를 생성하므로 화면상 배치가 완전히 동일해진다.
 *  2) [연산 순서 보장] "PCG 생성 완료" → "변경 영역(DirtyBounds) 수집" → "SVO 부분 재빌드"
 *     순서를 델리게이트 기반으로 강제한다. 모든 PCG 컴포넌트가 끝나기 전에는 SVO를 건드리지 않는다.
 *
 * 배치 방법:
 *  - 레벨에 이 액터 1개를 배치한다.
 *  - PCGActors 에 PCG 컴포넌트를 가진 액터(들)을 지정한다. (비우면 레벨 전체에서 자동 수집)
 *  - TargetSVOVolume 에 SVO 볼륨을 지정한다. (비우면 자동 탐색)
 *  - 각 PCG 컴포넌트의 GenerationTrigger 는 GenerateOnDemand 로 두는 것을 권장.
 */
UCLASS()
class ABYSSCRAWLER_API AAbyssPCGManager : public AActor
{
	GENERATED_BODY()

public:
	AAbyssPCGManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// 이 매니저가 제어할 PCG 액터. 비워두면 BeginPlay에서 레벨 전체를 자동 수집한다.
	UPROPERTY(EditAnywhere, Category = "Abyss PCG")
	TArray<TObjectPtr<AActor>> PCGActors;

	// 재빌드 대상 SVO 볼륨. 비워두면 자동 탐색.
	UPROPERTY(EditAnywhere, Category = "Abyss PCG")
	TObjectPtr<ASVOVolume> TargetSVOVolume;

	// 변경 영역을 SVO에 넘길 때 여유 마진(cm). 콜리전 경계 근처 보정용.
	UPROPERTY(EditAnywhere, Category = "Abyss PCG")
	float DirtyBoundsPadding = 200.0f;

	// 서버가 정해서 리플리케이트하는 시드. OnRep에서 클라 생성이 시작된다.
	UPROPERTY(ReplicatedUsing = OnRep_Seed)
	int32 ReplicatedSeed = 0;

	UFUNCTION()
	void OnRep_Seed();

private:
	// 시드 적용 → 전 PCG 생성 트리거 → 완료 델리게이트 바인딩
	void StartGeneration(int32 InSeed);

	// 개별 PCG 컴포넌트 생성 완료 콜백 (모두 끝나면 SVO 재빌드)
	void HandlePCGGenerated(UPCGComponent* InComponent);

	// PCGActors(또는 레벨 전체)에서 UPCGComponent들을 수집
	TArray<UPCGComponent*> CollectPCGComponents() const;

	// 아직 생성이 끝나지 않은 PCG 컴포넌트 수
	int32 PendingComponents = 0;

	// 모든 PCG가 변경한 영역의 합집합 (SVO에 전달)
	FBox AccumulatedDirtyBounds;

	// 중복 실행 방지
	bool bGenerationStarted = false;
};
