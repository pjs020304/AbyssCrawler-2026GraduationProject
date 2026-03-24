
#include "BTTask_FindRandomSVOPosition.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "SVOVolume.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_FindRandomSVOPosition::UBTTask_FindRandomSVOPosition()
{
	

}

EBTNodeResult::Type UBTTask_FindRandomSVOPosition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* Shark = OwnerComp.GetAIOwner()->GetPawn();
	ASVOVolume* SVOData = Cast<ASVOVolume>(UGameplayStatics::GetActorOfClass(GetWorld(), ASVOVolume::StaticClass()));

	if (!Shark || !SVOData) return EBTNodeResult::Failed;

	FVector Origin = Shark->GetActorLocation();
	FVector RandomPoint;
	bool bFound = false;

	// 최대 10번 정도 랜덤한 방향으로 레이를 쏴서 안전한 곳(Walkable)을 찾습니다.
	for (int i = 0; i < 10; ++i)
	{
		// 반경 2000 유닛 내의 랜덤 3D 좌표 생성
		FVector RandomDir = FMath::VRand();
		FVector CandidateLoc = Origin + (RandomDir * FMath::RandRange(500.0f, 2000.0f));

		// SVO 데이터베이스에 갈 수 있는 곳인지 물어봄
		if (SVOData->IsWalkable(CandidateLoc))
		{
			RandomPoint = CandidateLoc;
			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		// 찾은 좌표를 블랙보드의 'RandomLocation' 키에 저장합니다.
		OwnerComp.GetBlackboardComponent()->SetValueAsVector(RandomLocation.SelectedKeyName, RandomPoint);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}