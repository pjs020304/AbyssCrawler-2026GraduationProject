#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssSubmarine.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AAbyssItemBase;

// 잠수함 이동 구간. 서버가 한 번 정하면 모든 머신이 같은 서버 시간 기준으로 위치를 계산한다.
USTRUCT()
struct FAbyssSubmarineMove
{
	GENERATED_BODY()

	UPROPERTY()
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector EndLocation = FVector::ZeroVector;

	// GameState 서버 월드 시간 기준 출발 시각
	UPROPERTY()
	double StartServerTime = 0.0;

	UPROPERTY()
	float Duration = 0.0f;

	// 증가하는 번호. 같은 구간을 다시 받아도 구분하기 위함
	UPROPERTY()
	int32 MoveId = 0;
};

UCLASS()
class ABYSSCRAWLER_API AAbyssSubmarine : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()

public:
	AAbyssSubmarine();

	// [인터페이스 구현] E키 상호작용
	virtual void Interact_Implementation(AActor* InstigatorActor) override;

	// (선택) 시선이 닿았을 때 UI 표시용
	virtual void OnFocus_Implementation() override;
	virtual void OnLostFocus_Implementation() override;

	// [서버] 이동 중이고 아이템이 선체 안에 있으면 즉시 고정한다. 고정했으면 true.
	// 이동 중에 새로 떨어뜨린 아이템용 (AAbyssItemBase::SetAsDropped에서 호출)
	bool TryStowItem(AAbyssItemBase* Item);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- [추가됨] 시각적 컴포넌트 ---

	// 잠수함의 전체 외형 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SubmarineMesh;

	// 플레이어가 상호작용(E) 할 콘솔 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ConsoleMesh;

	// 상호작용 시 인원수를 표시할 텍스트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* InteractionText;

	// ------------------------------

	// 플레이어 탑승 확인용 구역
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* InteriorVolume;

	// 하강 목표 지점 (에디터에서 심해 좌표로 설정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (MakeEditWidget = true))
	FVector TargetLocation;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float DescentSpeed;

	// 현재 잠수함의 상태
	UPROPERTY(Replicated)
	bool bIsDescending;

	// 상승 중인지 여부
	UPROPERTY(Replicated)
	bool bIsAscending;

	// 초기 베이스캠프 위치
	FVector InitialLocation;

	// 현재(또는 마지막) 이동 구간. 이동 자체를 리플리케이트하지 않고 이 값만 보내서
	// 클라이언트도 매 프레임 부드럽게 같은 위치를 계산한다.
	// (예전에는 서버 위치를 ReplicateMovement로 받아 계단식으로 순간이동했고,
	//  그 위에 선 캐릭터/물리 아이템이 떨리다가 선체를 뚫고 빠져나갔다)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMove)
	FAbyssSubmarineMove CurrentMove;

	// 클라이언트에서 이동 구간을 재생 중인지
	bool bLocalMoveActive = false;

	UFUNCTION()
	void OnRep_CurrentMove();

	// [서버] Start → End 이동 시작
	void BeginMove(const FVector& EndLocation);

	// 이동 구간을 현재 서버 시간에 맞춰 적용. 도착했으면 true
	bool ApplyMoveAtCurrentTime();

	double GetSyncedWorldTime() const;

	// [서버] 이동 중 선체 안의 아이템을 선체에 고정 (물리 끄고 부착) / 도착 후 해제
	void StowCargo();
	void ReleaseCargo();

	// 아이템 위치가 선체 내부(InteriorVolume 박스 안)인지
	bool IsInsideInterior(const FVector& WorldLocation) const;

	// 이동 중 고정해 둔 아이템
	UPROPERTY()
	TArray<TObjectPtr<AAbyssItemBase>> StowedCargo;

	// 모든 플레이어가 탔는지 검사하는 함수
	bool AreAllPlayersBoarded();

	// 클리어 판정
	void TryClearGameByBoarding();

	// 실제 하강 로직 (서버 실행)
	void StartDescent();

	// 상승 시작 (서버 전용)
	void StartAscent();

	// 텍스트 업데이트 함수
	void UpdateInteractionText();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// InteriorVolume에 들어오고 나갈 때 실행될 이벤트 함수
	UFUNCTION()
	void OnInteriorOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnInteriorOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};