#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "CommonItemChoiceWidget.generated.h"

class UTextBlock;
class AMyPlayerState;
class UInventoryComponent;

/**
 * 보스 처치 보상 '흔함 아이템 선택권' UI
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UCommonItemChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- [ Public API (BP callable) ] ---
	/** 아이템 선택 시 BP에서 호출 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void MakeChoice(FName ChosenItemID);

protected:
	// --- [ Lifecycle ] ---
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BindDataSources();

	// --- [ Data Sources ] ---
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	UPROPERTY()
	TWeakObjectPtr<UInventoryComponent> InventoryComp;

	// --- [ UMG Binding ] ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceCountText;

	// --- [ Internal Handlers ] ---
	UFUNCTION()
	void HandleCommonItemChoiceCountChanged(int32 NewCount);

	void PopulateChoices();

	// --- [ Blueprint Implementable Events ] ---
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnShowChoices(const TArray<FItemData>& ItemChoices);

	/** 선택 완료 시 UI 숨김 요청 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnHideChoices();
};