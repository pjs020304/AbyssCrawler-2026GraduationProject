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
			UE_LOG(LogTemp, Warning, TEXT("[Loading] WidgetClass Loaded: %s"), *Loaded->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Loading] Failed to load widget class: %s"), DefaultWidgetPath);
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
	UE_LOG(LogTemp, Warning, TEXT("[Loading] PreLoadMap: %s"), *MapName);

	// 이전 로딩 위젯이 남아있으면 정리
	HideLoadingWidget();

	if (IsGameplayMap(MapName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Loading] Gameplay map detected. ShowMoviePlayer."));
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
	UE_LOG(LogTemp, Warning, TEXT("[Loading] PostLoadMap: World=%s"),
		LoadedWorld ? *LoadedWorld->GetName() : TEXT("NULL"));


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
	/*
	const bool bIsHostOrStandalone =
		(NetMode == NM_ListenServer) || (NetMode == NM_Standalone);
	if (bIsHostOrStandalone)
	{
		return;
	}
	*/
	// 여기부터 순수 클라이언트(NM_Client)
	ShowLoadingWidget(LoadedWorld);
}

void ULoadingScreenSubsystem::ShowLoadingWidget(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const int32 WorldKey = World->GetUniqueID();

	// 같은 월드에 이미 있으면 중복 생성 방지
	if (LoadingWidgets.Contains(WorldKey))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Loading] Already has widget for World=%s"), *World->GetName());
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();

	UE_LOG(LogTemp, Warning, TEXT("[Loading] ShowLoadingWidget World=%s PC=%s Class=%s"),
		*World->GetName(),
		PC ? *PC->GetName() : TEXT("NULL"),
		LoadingWidgetClass ? *LoadingWidgetClass->GetName() : TEXT("NULL"));

	if (!LoadingWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Loading] LoadingWidgetClass is NULL"));
		return;
	}

	ULoadingScreenWidget* NewWidget =
		CreateWidget<ULoadingScreenWidget>(World->GetGameInstance(), LoadingWidgetClass);

	if (!NewWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[Loading] CreateWidget failed"));
		return;
	}

	NewWidget->AddToViewport(99999);
	NewWidget->OnProgressUpdated(0.f);

	LoadingWidgets.Add(WorldKey, NewWidget);
	LoadingWorlds.Add(WorldKey, World);
	LoadingPlayerControllers.Add(WorldKey, PC);
	DisplayedProgressMap.Add(WorldKey, 0.f);
	LoadingStartTimeMap.Add(WorldKey, FPlatformTime::Seconds());

	if (PC)
	{
		FInputModeUIOnly Mode;
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = false;
	}

	if (!TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &ULoadingScreenSubsystem::Tick), 0.f);
	}
}

void ULoadingScreenSubsystem::HideLoadingWidget()
{
	for (auto& Pair : LoadingWidgets)
	{
		if (ULoadingScreenWidget* Widget = Pair.Value)
		{
			Widget->OnLoadingFinished();
			Widget->RemoveFromParent();
		}
	}

	LoadingWidgets.Empty();
	LoadingWorlds.Empty();
	LoadingPlayerControllers.Empty();
	DisplayedProgressMap.Empty();
	LoadingStartTimeMap.Empty();

	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
}

// ──────────────────────────────────────────────────────────────
// 폴링: 준비 완료 여부를 확인하고 진행률 바를 스무딩한다.
// ──────────────────────────────────────────────────────────────
bool ULoadingScreenSubsystem::Tick(float DeltaTime)
{
	TArray<int32> KeysToRemove;

	for (auto& Pair : LoadingWidgets)
	{
		const int32 WorldKey = Pair.Key;
		ULoadingScreenWidget* Widget = Pair.Value;

		UWorld* World = nullptr;
		if (TObjectPtr<UWorld>* FoundWorld = LoadingWorlds.Find(WorldKey))
		{
			World = FoundWorld->Get();
		}

		if (!Widget || !World)
		{
			KeysToRemove.Add(WorldKey);
			continue;
		}

		const bool bReady = IsWorldReady(World);
		const double StartTime = LoadingStartTimeMap.Contains(WorldKey)
			? LoadingStartTimeMap[WorldKey]
			: FPlatformTime::Seconds();

		const double Elapsed = FPlatformTime::Seconds() - StartTime;

		float& DisplayedProgress = DisplayedProgressMap.FindOrAdd(WorldKey);
		const float Target = bReady ? 1.f : ProgressHoldCap;

		DisplayedProgress = FMath::FInterpConstantTo(
			DisplayedProgress,
			Target,
			DeltaTime,
			ProgressInterpSpeed
		);

		Widget->OnProgressUpdated(DisplayedProgress);

		if (bReady && DisplayedProgress >= 0.999f && Elapsed >= MinDisplayTime)
		{
			Widget->OnLoadingFinished();
			Widget->RemoveFromParent();

			if (TWeakObjectPtr<APlayerController>* FoundPC = LoadingPlayerControllers.Find(WorldKey))
			{
				if (APlayerController* PC = FoundPC->Get())
				{
					PC->SetInputMode(FInputModeGameOnly());
					PC->bShowMouseCursor = false;
				}
			}

			KeysToRemove.Add(WorldKey);
		}
	}

	for (int32 Key : KeysToRemove)
	{
		LoadingWidgets.Remove(Key);
		LoadingWorlds.Remove(Key);
		LoadingPlayerControllers.Remove(Key);
		DisplayedProgressMap.Remove(Key);
		LoadingStartTimeMap.Remove(Key);
	}

	if (LoadingWidgets.Num() <= 0)
	{
		TickHandle.Reset();
		return false;
	}

	return true;
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
