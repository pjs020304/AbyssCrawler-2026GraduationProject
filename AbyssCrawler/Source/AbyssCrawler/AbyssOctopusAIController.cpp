#include "AbyssOctopusAIController.h"
#include "AbyssOctopusCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "DecoyActor.h"

AAbyssOctopusAIController::AAbyssOctopusAIController()
{
	OctopusPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("OctopusPerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 900.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	OctopusPerceptionComp->ConfigureSense(*SightConfig);
	OctopusPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	//OctopusPerceptionComp->SetDominantSense(SightConfig->GetSenseID());
	OctopusPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAbyssOctopusAIController::OnTargetDetected);
}

void AAbyssOctopusAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AIBehavior)
	{
		RunBehaviorTree(AIBehavior);
	}
}

void AAbyssOctopusAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	if (GetBlackboardComponent())
	{
		AActor* CurrentTarget = Cast<AActor>(GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")));
		if (CurrentTarget && CurrentTarget->IsA(ADecoyActor::StaticClass()))
		{
			return; // Do not override target if currently chasing decoy
		}
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// Detected someone, disable stealth
		if (AAbyssOctopusCharacter* Octopus = Cast<AAbyssOctopusCharacter>(GetPawn()))
		{
			Octopus->SetStealthMode(false);
		}

		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->SetValueAsObject("TargetActor", Actor);
		}
	}
	else
	{
		// Lost sight
		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->ClearValue("TargetActor");
		}
		
		// Re-enable stealth if desired when player escapes
		if (AAbyssOctopusCharacter* Octopus = Cast<AAbyssOctopusCharacter>(GetPawn()))
		{
			Octopus->SetStealthMode(true);
		}
	}
}
