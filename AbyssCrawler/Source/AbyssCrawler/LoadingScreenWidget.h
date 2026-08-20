// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreenWidget.generated.h"

/**
 * 로딩 화면 UMG의 C++ 베이스.
 * WBP_LoadingScreen 을 이 클래스로 Reparent 한 뒤,
 * OnProgressUpdated / OnLoadingFinished 이벤트를 그래프에서 구현해
 * ProgressBar, 텍스트, 페이드 아웃 등을 처리하면 된다.
 */
UCLASS(Abstract)
class ABYSSCRAWLER_API ULoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 진행률(0.0 ~ 1.0)이 갱신될 때마다 서브시스템이 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Loading")
	void OnProgressUpdated(float Progress);

	// 로딩이 끝나 위젯이 제거되기 직전에 호출된다. (페이드 아웃 애니메이션 등에 사용)
	UFUNCTION(BlueprintImplementableEvent, Category = "Loading")
	void OnLoadingFinished();
};
