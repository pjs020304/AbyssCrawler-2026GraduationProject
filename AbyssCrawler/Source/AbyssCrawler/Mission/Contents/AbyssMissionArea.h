// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssMissionArea.generated.h"

class UBoxComponent;
class UWidgetComponent;
class UStaticMeshComponent;

UCLASS()
class ABYSSCRAWLER_API AAbyssMissionArea : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbyssMissionArea();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void SetMissionMarkerVisible(bool bVisible);

	UFUNCTION(BlueprintPure)
	FName GetMissionId() const { return MissionId; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* AreaCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UWidgetComponent* MissionMarkerWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FName MissionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FVector MarkerOffset = FVector(0.0f, 0.0f, 250.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FVector AreaExtent = FVector(600.0f, 600.0f, 300.0f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission|Marker")
	TObjectPtr<UStaticMeshComponent> MissionArrowMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	FVector ArrowOffset = FVector(0.0f, 0.0f, 300.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float ArrowRotationSpeed = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float ArrowBobAmplitude = 80.0f; // 위아래 움직이는 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float ArrowBobSpeed = 2.0f; // 움직이는 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float ArrowHideDistance = 300.0f;

	UPROPERTY()
	FVector ArrowBaseLocation;

	UPROPERTY(ReplicatedUsing = OnRep_MissionMarkerVisible)
	bool bMissionMarkerVisible = false;

	UPROPERTY(Replicated)
	bool bTriggered = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Marker")
	float MarkerHideDistance = 1200.0f;

	UFUNCTION()
	void OnRep_MissionMarkerVisible();

	UFUNCTION()
	void OnAreaBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void ApplyMarkerVisible();

	virtual void Tick(float DeltaTime) override;

	void UpdateMarkerDistanceVisibility();
};
