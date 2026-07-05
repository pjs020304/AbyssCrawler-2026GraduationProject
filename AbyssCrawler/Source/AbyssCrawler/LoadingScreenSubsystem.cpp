// Fill out your copyright notice in the Description page of Project Settings.

#include "LoadingScreenSubsystem.h"
#include "LoadingScreenWidget.h"

#include "MoviePlayer.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

const TCHAR* ULoadingScreenSubsystem::DefaultWidgetPath =
	TEXT("/Game/AbyssCrawler/Core/UI/WBP_LoadingScreen.WBP_LoadingScreen_C");

void ULoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 기본 위젯 경로가 존재하면 미리 로드해 둔다. (없으면 무시 — 진행률 바 없이 로직만 동작)
	if (!LoadingWidgetClass)
	{
		if (UClass* Loaded = LoadClass<ULoadingScreenWidget>(nullptr, DefaultWidgetPath))
		{
			LoadingWidgetClass = Loaded;
		}
	}

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ULoadingScreenSubsystem::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULoadingScreenSubsystem::HandlePostLoadMap);
}

void ULoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	Super::Deinitialize();
}

void ULoadingScreenSubsystem::SetLoadingWidgetClass(TSubclassOf<ULoadingScreenWidget> InClass)
{
	if (InClass)
	{
		LoadingWidgetClass = InClass;
	}
}

// ──────────────────────────────────────────────────────────────
// A 단계: 맵 패키지 로드 직전. 게임 스레드가 곧 블로킹되므로
//         별도 스레드로 그려지는 MoviePlayer 로딩 화면을 띄운다.
// ──────────────────────────────────────────────────────────────
void ULoadingScreenSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// 이전 로딩 위젯이 남아있으면 정리
	HideLoadingWidget();

	if (IsGameplayMap(MapName))
	{
		ShowMoviePlayer();
	}
}

void ULoadingScreenSubsystem::ShowMoviePlayer()
{
	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	if (!MoviePlayer)
	{
		return;
	}

	FLoadingScreenAttributes Attr;
	Attr.bAutoCompleteWhenLoadingCompletes = true; // 패키지 로드가 끝나면 자동 종료
	Attr.bMoviesAreSkippable = false;
	Attr.MinimumLoadingScreenDisplayTime = 0.5f;
	// 임시 슬레이트 스피너. 실제 무비(Attr.MoviePaths)나 커스텀 슬레이트 위젯으로 교체 가능.
	Attr.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

	MoviePlayer->SetupLoadingScreen(Attr);
}

// ──────────────────────────────────────────────────────────────
// B 단계: 맵 로드 완료. 게임 스레드가 살아있다.
//         NetMode 분기 후 클라이언트에서만 UMG 진행률 바를 띄운다.
// ──────────────────────────────────────────────────────────────
void ULoadingScreenSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || !LoadedWorld->IsGameWorld())
	{
		return;
	}

	// 게임플레이 맵이 아니면(로비/타이틀 등) 로딩 화면 없음
	if (!IsGameplayMap(LoadedWorld->GetMapName()))
	{
		return;
	}

	const ENetMode NetMode = LoadedWorld->GetNetMode();

	// 데디케이티드 서버: 로컬 플레이어/뷰포트가 없으므로 UI 없음
	if (NetMode == NM_DedicatedServer)
	{
		return;
	}

	// 리슨서버 호스트 / Standalone(권한 보유): 렌더가 빠르므로 진행률 화면 스킵.
	// MoviePlayer 는 패키지 로드가 끝나면 자동으로 사라진다.
	const bool bIsHostOrStandalone =
		(NetMode == NM_ListenServer) || (NetMode == NM_Standalone);
	if (bIsHostOrStandalone)
	{
		return;
	}

	// 여기부터 순수 클라이언트(NM_Client)
	ShowLoadingWidget(LoadedWorld);
}

void ULoadingScreenSubsystem::ShowLoadingWidget(UWorld* World)
{
	DisplayedProgress = 0.f;
	bLoadingActive = true;
	LoadingStartTime = FPlatformTime::Seconds();

	APlayerController* PC = World->GetFirstPlayerController();

	if (LoadingWidgetClass)
	{
		LoadingWidget = CreateWidget<ULoadingScreenWidget>(World->GetGameInstance(), LoadingWidgetClass);
		if (LoadingWidget)
		{
			LoadingWidget->AddToViewport(1000); // 최상단
			LoadingWidget->OnProgressUpdated(0.f);
		}
	}

	if (PC)
	{
		FInputModeUIOnly Mode;
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = false;
	}

	// 폴링 시작
	if (!TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &ULoadingScreenSubsystem::Tick), 0.f);
	}
}

void ULoadingScreenSubsystem::HideLoadingWidget()
{
	bLoadingActive = false;

	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	if (LoadingWidget)
	{
		LoadingWidget->OnLoadingFinished();
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}

	// 인풋 복구
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

// ──────────────────────────────────────────────────────────────
// 폴링: 준비 완료 여부를 확인하고 진행률 바를 스무딩한다.
// ──────────────────────────────────────────────────────────────
bool ULoadingScreenSubsystem::Tick(float DeltaTime)
{
	if (!bLoadingActive)
	{
		return false; // 티커 제거
	}

	UWorld* World = GetWorld();
	const bool bReady = IsWorldReady(World);
	const double Elapsed = FPlatformTime::Seconds() - LoadingStartTime;

	// 완료 전에는 상한(0.9)까지만 부드럽게 채우고, 완료되면 1.0으로 채운다.
	const float Target = bReady ? 1.f : ProgressHoldCap;
	DisplayedProgress = FMath::FInterpConstantTo(DisplayedProgress, Target, DeltaTime, ProgressInterpSpeed);

	if (LoadingWidget)
	{
		LoadingWidget->OnProgressUpdated(DisplayedProgress);
	}

	// 완료 && 바가 다 참 && 최소 노출 시간 경과 -> 종료
	if (bReady && DisplayedProgress >= 0.999f && Elapsed >= MinDisplayTime)
	{
		HideLoadingWidget();
		return false; // 티커 종료
	}

	return true; // 계속 폴링
}

// B 단계 완료 판정
bool ULoadingScreenSubsystem::IsWorldReady(UWorld* World) const
{
	if (!World)
	{
		return true;
	}

	// 1) World Partition 을 쓴다면 셀 스트리밍 완료 여부 확인. (단일 맵이라 서브시스템이 없으면 스킵)
	if (UWorldPartitionSubsystem* WPSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>())
	{
		if (!WPSubsystem->IsAllStreamingCompleted())
		{
			return false;
		}
	}

	// 2) 서버가 내 폰을 possess 했는가? (실제로 캐릭터를 조작/렌더 가능한 상태)
	//    리슨서버에서 클라가 "느리게 렌더된다"고 느끼는 구간이 대부분 여기다.
	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return false;
	}

	// 3) GameState 리플리케이션 정착 (초기 상태 동기화 완료 신호)
	if (!World->GetGameState())
	{
		return false;
	}

	return true;
}

bool ULoadingScreenSubsystem::IsGameplayMap(const FString& MapName) const
{
	// PIE에서는 "UEDPIE_0_L_DeepSea_VerLightStudio" 처럼 접두어가 붙으므로 부분 일치로 검사.
	for (const FString& Keyword : GameplayMapKeywords)
	{
		if (MapName.Contains(Keyword))
		{
			return true;
		}
	}
	return false;
}
