#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"

// 패키징 빌드에서 크리처가 플레이어를 전혀 감지하지 못하던 문제 대응.
//
// 에디터(PIE)는 게임 맵을 하드 LoadMap으로 열지만, 패키징 빌드는 로비에서
// seamless travel(LobbyGameMode::bUseSeamlessTravel)로 들어온다. 이 경로에서는
// 월드 초기화 순서가 달라져, AI 컨트롤러의 AIPerceptionComponent::OnRegister가
// 돌 때 월드에 AISystem(=PerceptionSystem)이 아직 없으면 감각(Sight) 등록과
// 리스너 등록이 조용히 건너뛰어진다. 그러면 BT(순찰)는 정상인데
// OnTargetPerceptionUpdated가 한 번도 오지 않아 추적/공격이 전혀 일어나지 않는다.
//
// 빙의 시점에 리스너 ID가 유효한지 확인하고, 아니면 컴포넌트를 다시 등록해
// OnRegister의 감각/리스너 등록을 재실행한다.
inline void EnsurePerceptionListenerRegistered(UAIPerceptionComponent* PerceptionComp)
{
	if (!PerceptionComp)
	{
		return;
	}

	if (PerceptionComp->GetListenerId() != FPerceptionListenerID::InvalidID())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AIPerception] %s: listener was not registered, re-registering perception component"),
		*GetNameSafe(PerceptionComp->GetOwner()));

	if (PerceptionComp->IsRegistered())
	{
		PerceptionComp->UnregisterComponent();
	}
	PerceptionComp->RegisterComponent();
}
