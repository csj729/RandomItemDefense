// Source/RamdomItemDefense/Public/MainHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class AMyPlayerState;
class ARamdomItemDefensePlayerController;
class UTextBlock; // [헤더 추가]

UCLASS()
class RAMDOMITEMDEFENSE_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;
	// [추가] 매 프레임 UI 갱신을 위해 오버라이드
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BindPlayerStateEvents();
	void BindSpawnerEvents();

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "Controller")
	TObjectPtr<ARamdomItemDefensePlayerController> MyPlayerController;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> P1_MonsterCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> P2_MonsterCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> P1_NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> P2_NameText;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnGoldChanged(int32 NewGold);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnMonsterCountChanged(int32 NewCount, int32 MaxCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnUltimateChargeChanged(int32 NewValue, int32 MaxValue);

	UFUNCTION(BlueprintCallable, Category = "UI Actions")
	void HandleStatUpgradeClicked();

	UFUNCTION(BlueprintCallable, Category = "UI Actions")
	void HandleInventoryClicked();

	UFUNCTION()
	void HandleSpawnerAssigned(int32 Dummy);

	UFUNCTION()
	void HandleMonsterCountChanged(int32 NewCount);

	UFUNCTION()
	void HandleUltimateChargeChanged(int32 NewValue);

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Button Action")
	void ShowButtonActionPrompt(EButtonActionKey KeyToPress, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Button Action")
	void HideButtonActionPrompt();

	UFUNCTION(BlueprintImplementableEvent, Category = "Button Action")
	void ShowButtonActionResult(bool bWasSuccess, int32 RewardIndex);

private:
	// [추가] 몬스터 현황 업데이트 헬퍼 함수
	void UpdatePVPMonsterCounts();
};