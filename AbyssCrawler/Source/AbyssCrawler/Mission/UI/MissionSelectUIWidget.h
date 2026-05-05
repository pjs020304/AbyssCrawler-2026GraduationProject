#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbyssGameState.h"
#include "MissionSelectUIWidget.generated.h"

class UVerticalBox;
class UButton;
class UMissionSelectSlotWidget;

UCLASS()
class ABYSSCRAWLER_API UMissionSelectUIWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void InitializeMissionList(const TArray<FAbyssMissionData>& Missions);

protected:
	virtual void NativeConstruct() override;

	// UI
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* MissionList;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Confirm;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Cancel;

	UPROPERTY(EditAnywhere, Category = "Mission")
	TSubclassOf<UMissionSelectSlotWidget> MissionSlotClass;

	// 선택된 미션
	UPROPERTY()
	TArray<FName> SelectedMissionIds;

	UPROPERTY(EditAnywhere)
	int32 MaxSelectableCount = 3;

	// 슬롯 클릭 이벤트
	UFUNCTION()
	void HandleMissionSelected(FName MissionId);

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	bool bIsClosing = false;

	void CloseUI();

};
