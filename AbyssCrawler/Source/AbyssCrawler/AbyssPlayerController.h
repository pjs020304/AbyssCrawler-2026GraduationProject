#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AbyssPlayerController.generated.h"

class ALevelSequenceActor;
class UUserWidget;

UCLASS()
class ABYSSCRAWLER_API AAbyssPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAbyssPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(Client, Reliable)
	void Client_StartEndingSequence(TSubclassOf<UUserWidget> ClearWidgetClass);

	UFUNCTION()
	void HandleEndingSequenceFinished();

	void ShowEndingClearUI();

	UPROPERTY()
	TSubclassOf<UUserWidget> PendingEndingWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> EndingWidgetRef;

	// 게임 오버 UI 표시.
	// 캐릭터가 아니라 컨트롤러로 보내야 이미 죽어 관전 중인(폰이 없는) 플레이어에게도 도착한다.
	UFUNCTION(Client, Reliable)
	void Client_ShowGameOverUI(TSubclassOf<UUserWidget> GameOverWidgetClass);

	// [서버] 사망 → 관전 상태 진입 + 살아있는 팀원에게 카메라 연결
	void StartSpectating();

protected:
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	// 좌클릭 입력 → 서버에 다음 관전 대상 요청
	void CycleSpectatePlayer();

	UFUNCTION(Server, Reliable)
	void Server_CycleSpectateTarget();

	// [서버] 현재 대상 다음의 생존 팀원을 골라 관전 대상으로 지정 (없으면 nullptr)
	void SelectNextSpectateTarget();

	void SetSpectateTarget(APawn* NewTarget);

	// 관전 대상에 카메라 적용 (로컬 컨트롤러 전용)
	void ApplySpectateViewTarget(bool bBlend);

	bool IsSpectating() const;

	UFUNCTION()
	void OnRep_SpectateTarget();

	UPROPERTY()
	TObjectPtr<UUserWidget> GameOverWidgetRef;

	// 현재 관전 중인 팀원. 서버가 결정하고 소유 클라이언트에만 복제된다.
	UPROPERTY(ReplicatedUsing = OnRep_SpectateTarget)
	TObjectPtr<APawn> SpectateTarget;
};
