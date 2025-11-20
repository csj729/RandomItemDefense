#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundChoiceWidget.generated.h"

// 전방 선언
class UTextBlock;
class UButton;
class AMyPlayerState;

UCLASS()
class RAMDOMITEMDEFENSE_API URoundChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 위젯 생성 시 '단 한 번' 호출됩니다. (버튼 바인딩용) */
	virtual void NativeOnInitialized() override;

	/** 위젯이 뷰포트에 추가될 때 '매번' 호출됩니다. (델리게이트 바인딩용) */
	virtual void NativeConstruct() override;

	/** 위젯이 뷰포트에서 제거될 때 '매번' 호출됩니다. (델리게이트 해제용) */
	virtual void NativeDestruct() override;

	void BindPlayerState();

	/** PlayerState 참조 */
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	// --- UMG 디자이너 위젯 바인딩 ---
protected:
	/** "남은 선택 횟수: X" 텍스트 (UMG 이름: ChoiceCountText) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceCountText;

	/** [아이템 뽑기] 버튼 (UMG 이름: ItemGachaButton) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ItemGachaButton;

	/** [골드 도박] 버튼 (UMG 이름: GoldGambleButton) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GoldGambleButton;

	// --- C++ 이벤트 핸들러 ---
protected:
	/** PlayerState의 선택 횟수가 변경될 때 호출됩니다. */
	UFUNCTION()
	void HandleChoiceCountChanged(int32 NewCount);

	/** [아이템 뽑기] 버튼 클릭 시 호출됩니다. */
	UFUNCTION()
	void HandleItemGachaClicked();

	/** [골드 도박] 버튼 클릭 시 호출됩니다. */
	UFUNCTION()
	void HandleGoldGambleClicked();
};