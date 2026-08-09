// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "LoadingScreenSubsystem.generated.h"

class ULoadingScreenWidget;

/**
 * 리슨서버 환경에서 맵 진입 로딩 화면을 관리하는 GameInstance 서브시스템.
 *
 * 로딩은 성격이 다른 2단계로 나뉜다.
 *   A. 블로킹 패키지 로드(Travel) : 게임 스레드가 멈춘다 -> MoviePlayer(별도 스레드)로 표시.
 *   B. possess / 리플리케이션 정착(+파티션 스트리밍) : 게임 스레드 살아있음 -> UMG 진행률 바로 표시.
 *
 * NetMode 분기:
 *   - Dedicated Server        : 로컬 플레이어가 없으므로 아무 것도 하지 않음.
 *   - ListenServer / Standalone(권한 보유) : 렌더가 빠르므로 UMG 진행률 화면은 스킵(호스트 빠른 진입).
 *   - Client                  : MoviePlayer + UMG 진행률 바 전체 수행.
 */
UCLASS()
class ABYSSCRAWLER_API ULoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// BP_GameInstance 등에서 위젯 클래스를 주입하고 싶을 때 사용. (미설정 시 아래 기본 경로를 로드)
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void SetLoadingWidgetClass(TSubclassOf<ULoadingScreenWidget> InClass);

private:
	// --- 델리게이트 핸들러 ---
	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	// --- 로딩 화면 제어 ---
	void ShowMoviePlayer();
	void ShowLoadingWidget(UWorld* World);
	void HideLoadingWidget();

	// --- 폴링 ---
	bool Tick(float DeltaTime);           // FTSTicker 콜백
	bool IsWorldReady(UWorld* World) const; // B 단계 완료 판정

	// --- 헬퍼 ---
	bool IsGameplayMap(const FString& MapName) const;

	// 현재 진행률 화면이 표시할 대상 위젯
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ULoadingScreenWidget>> LoadingWidgets;

	// 표시할 위젯 클래스 (SetLoadingWidgetClass 또는 기본 경로 로드로 채워짐)
	UPROPERTY(Transient)
	TSubclassOf<ULoadingScreenWidget> LoadingWidgetClass;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UWorld>> LoadingWorlds;

	TMap<int32, TWeakObjectPtr<APlayerController>> LoadingPlayerControllers;
	TMap<int32, float> DisplayedProgressMap;
	TMap<int32, double> LoadingStartTimeMap;

	FTSTicker::FDelegateHandle TickHandle;

	// ─── 설정값(필요 시 여기만 수정) ───────────────────────────
	// 진행률 바가 실제 완료 전까지 도달할 상한 (완료 시 1.0으로 스냅)
	static constexpr float ProgressHoldCap = 0.9f;
	// 진행률 바 채워지는 속도 (초당)
	static constexpr float ProgressInterpSpeed = 0.6f;
	// 로딩 화면 최소 노출 시간(초). 순식간에 깜빡이는 것을 방지.
	static constexpr float MinDisplayTime = 0.75f;

	// 기본 위젯 애셋 경로. 이 경로에 WBP_LoadingScreen(ULoadingScreenWidget 상속)을 만들어두면 자동 로드된다.
	static const TCHAR* DefaultWidgetPath;

	// 로딩 화면을 띄울 게임플레이 맵 판별용 키워드. (부분 일치)
	// L_DeepSea, L_DeepSea_VerLightStudio 모두 매칭된다. Test 맵도 필요하면 추가.
	inline static const TArray<FString> GameplayMapKeywords = { TEXT("L_DeepSea") };
};
