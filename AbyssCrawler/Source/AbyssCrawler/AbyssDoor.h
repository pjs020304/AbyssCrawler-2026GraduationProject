#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssInteractionInterface.h"
#include "AbyssDoor.generated.h"

UCLASS()
class ABYSSCRAWLER_API AAbyssDoor : public AActor, public IAbyssInteractionInterface
{
	GENERATED_BODY()

public:
	AAbyssDoor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door Components")
	USceneComponent* RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door Components")
	UStaticMeshComponent* DoorFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door Components")
	UStaticMeshComponent* DoorMesh;

	// --- 인터페이스 함수 오버라이드 ---
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// 로컬 변수
	UPROPERTY(ReplicatedUsing = OnRep_DoorState)
	bool bIsOpen;

	// [New] 문이 열리는 각도의 크기 (무조건 양수로 90도)
	UPROPERTY(EditAnywhere, Category = "Door Settings")
	float OpenAngle = 90.0f;

	// [New] 문이 열리는 방향 (1.0f 또는 -1.0f)
	UPROPERTY(Replicated)
	float DirectionMultiplier;

	// 현재 문의 회전 각도
	float CurrentYaw;

	UFUNCTION()
	void OnRep_DoorState();
};