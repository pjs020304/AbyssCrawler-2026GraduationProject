#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssSubmarine.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

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