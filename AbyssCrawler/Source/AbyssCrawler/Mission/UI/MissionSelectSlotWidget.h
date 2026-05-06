#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbyssGameState.h"
#include "MissionSelectSlotWidget.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionSelectClicked, FName, MissionId);

UCLASS()
class ABYSSCRAWLER_API UMissionSelectSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetMissionData(const FAbyssMissionData& InMissionData);

	UPROPERTY(BlueprintAssignable)
	FOnMissionSelectClicked OnMissionSelectClicked;

	FName GetMissionId() const { return CachedMissionId; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_MissionName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_MissionInfo;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_Reward;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Select;

	UFUNCTION()
	void HandleSelectClicked();

private:
	FName CachedMissionId;

	
	
};
