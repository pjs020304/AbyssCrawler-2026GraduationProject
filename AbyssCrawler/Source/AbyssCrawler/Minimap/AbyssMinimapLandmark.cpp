#include "Minimap/AbyssMinimapLandmark.h"

#include "Components/BillboardComponent.h"

AAbyssMinimapLandmark::AAbyssMinimapLandmark()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->SetupAttachment(RootComponent);
	}

	// 월드 파티션: 항상 로드 (서버가 어디 있든 미니맵 수집 대상에 포함되도록)
	bIsSpatiallyLoaded = false;
#endif
}
