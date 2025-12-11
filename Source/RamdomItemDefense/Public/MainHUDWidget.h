#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class AMyPlayerState;
class ARamdomItemDefensePlayerController;
class UTextBlock;

UCLASS()
class RAMDOMITEMDEFENSE_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- [ Button Action UI (Implemented in BP) ] ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Button Action")
	void ShowButtonActionPrompt(EButtonActionKey KeyToPress, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Button Action")
	void HideButtonActionPrompt();

	UFUNCTION(BlueprintImplementableEvent, Category = "Button Action")
	void ShowButtonActionResult(bool bWasSuccess, int32 RewardIndex);

protected:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BindPlayerStateEvents();
	void BindSpawnerEvents();

	// --- [ References ] ---
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "Controller")
	TObjectPtr<ARamdomItemDefensePlayerController> MyPlayerController;

	// --- [ UMG Widgets ] ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> P1_MonsterCountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> P2_MonsterCountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> P1_NameText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> P2_NameText;

	// --- [ BP Events ] ---
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events") void OnGoldChanged(int32 NewGold);
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events") void OnMonsterCountChanged(int32 NewCount, int32 MaxCount);
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events") void OnUltimateChargeChanged(int32 NewValue, int32 MaxValue);

	// --- [ UI Actions ] ---
	UFUNCTION(BlueprintCallable, Category = "UI Actions") void HandleStatUpgradeClicked();
	UFUNCTION(BlueprintCallable, Category = "UI Actions") void HandleInventoryClicked();

	// --- [ Handlers ] ---
	UFUNCTION() void HandleSpawnerAssigned(int32 Dummy);
	UFUNCTION() void HandleMonsterCountChanged(int32 NewCount);
	UFUNCTION() void HandleUltimateChargeChanged(int32 NewValue);

private:
	void UpdatePVPMonsterCounts();
};