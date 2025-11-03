#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class AMyPlayerState;
class ARamdomItemDefensePlayerController;

UCLASS()
class RAMDOMITEMDEFENSE_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 위젯이 처음 생성될 때 호출됩니다.
	virtual bool Initialize() override;

	// PlayerState의 델리게이트에 이벤트를 바인딩(연결)합니다.
	void BindPlayerStateEvents();

	void BindSpawnerEvents();

	// PlayerState 참조
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	// PlayerController 참조
	UPROPERTY(BlueprintReadOnly, Category = "Controller")
	TObjectPtr<ARamdomItemDefensePlayerController> MyPlayerController;

	// --- 블루프린트(UMG)에서 구현할 이벤트 ---

		/** (WBP_MainHUD에서 구현) 골드 값이 변경될 때 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnGoldChanged(int32 NewGold);

	/** (WBP_MainHUD에서 구현) 몬스터 수가 변경될 때 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnMonsterCountChanged(int32 NewCount, int32 MaxCount);

	/** (WBP_MainHUD에서 구현) 궁극기 스택이 변경될 때 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnUltimateChargeChanged(int32 NewValue, int32 MaxValue);

	// --- 블루프린트(UMG)의 버튼이 클릭할 함수 ---

	/** (WBP_MainHUD의 '스탯 강화' 버튼에 연결) */
	UFUNCTION(BlueprintCallable, Category = "UI Actions")
	void HandleStatUpgradeClicked();

	/** (WBP_MainHUD의 '인벤토리' 버튼에 연결) */
	UFUNCTION(BlueprintCallable, Category = "UI Actions")
	void HandleInventoryClicked();

	/** PlayerState에 스포너가 할당/복제되었을 때 C++에서 수신 */
	UFUNCTION()
	void HandleSpawnerAssigned(int32 Dummy); // PlayerState의 OnSpawnerAssignedDelegate에 연결

	/** 스포너의 몬스터 수가 변경될 때 C++에서 수신 */
	UFUNCTION()
	void HandleMonsterCountChanged(int32 NewCount);

	/** PlayerState의 궁극기 스택이 변경될 때 C++에서 수신 */
	UFUNCTION()
	void HandleUltimateChargeChanged(int32 NewValue);
};