// Source/RamdomItemDefense/Public/CommonItemChoiceWidget.h (새 파일)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h" // FItemData
#include "CommonItemChoiceWidget.generated.h"

class UTextBlock;
class AMyPlayerState;
class ARamdomItemDefenseCharacter;
class UInventoryComponent;

/**
 * @brief 보스 처치 보상으로 '흔함 아이템 선택권'을 사용할 때 표시되는 UI
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UCommonItemChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 위젯이 뷰포트에 추가될 때 (델리게이트 바인딩) */
	virtual void NativeConstruct() override;

	/** 위젯이 뷰포트에서 제거될 때 (델리게이트 해제) */
	virtual void NativeDestruct() override;

	/** PlayerState 참조 */
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	/** InventoryComponent 참조 (아이템 정보 가져오기용) */
	UPROPERTY()
	TWeakObjectPtr<UInventoryComponent> InventoryComp;

	// --- UMG 위젯 바인딩 ---
protected:
	/** "남은 선택 횟수: X" 텍스트 (UMG 이름: ChoiceCountText) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceCountText;

	// --- C++ 이벤트 핸들러 ---
protected:
	/** PlayerState의 '흔함 아이템 선택권' 횟수가 변경될 때 호출됩니다. */
	UFUNCTION()
	void HandleCommonItemChoiceCountChanged(int32 NewCount);

	/**
	 * @brief (C++ 내부용) 선택권이 생겼을 때, BP에게 표시할 아이템 목록을 요청합니다.
	 */
	void PopulateChoices();

	// --- 블루프린트(WBP) 구현용 ---
protected:
	/**
	 * @brief (블루프린트 구현용) C++이 3개의 아이템 정보를 보내면,
	 * 이 이벤트가 호출되어 WBP에서 3개의 아이템 버튼(슬롯)을 채워야 합니다.
	 * @param ItemChoices UI에 표시할 3개의 아이템 데이터 배열
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnShowChoices(const TArray<FItemData>& ItemChoices);

	/**
	 * @brief (블루프린트 구현용) 선택권이 0개가 되거나, 선택을 완료했을 때
	 * 아이템 버튼(슬롯)을 숨기거나 비활성화할 것을 요청합니다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnHideChoices();

	// --- 블루프린트(WBP) 호출용 ---
public:
	/**
	 * @brief (블루프린트 호출용) WBP의 아이템 버튼 클릭 시 이 함수를 호출해야 합니다.
	 * @param ChosenItemID 플레이어가 선택한 아이템의 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void MakeChoice(FName ChosenItemID);
};