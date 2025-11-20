#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;

UCLASS()
class RAMDOMITEMDEFENSE_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 데이터 소스(인벤토리) 연결을 시도하고 실패 시 재시도하는 함수 */
	void BindDataSources();

	// --- 데이터 소스 ---
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	TObjectPtr<UInventoryComponent> InventoryComp;

	// [삭제됨] MyPlayerState (골드는 MainHUD에서 관리)

	// --- C++ 내부 핸들러 ---
	UFUNCTION()
	void HandleInventoryUpdated();

	// [삭제됨] HandleGoldChanged

public:
	// --- [블루프린트 구현용 이벤트] ---
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnInventoryUpdated();

};