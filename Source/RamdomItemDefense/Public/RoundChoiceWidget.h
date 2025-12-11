#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundChoiceWidget.generated.h"

class UTextBlock;
class UButton;
class AMyPlayerState;

UCLASS()
class RAMDOMITEMDEFENSE_API URoundChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BindPlayerState();

	// --- [ References ] ---
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	// --- [ UMG Binding ] ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ChoiceCountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ItemGachaButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> GoldGambleButton;

	// --- [ Handlers ] ---
	UFUNCTION() void HandleChoiceCountChanged(int32 NewCount);
	UFUNCTION() void HandleItemGachaClicked();
	UFUNCTION() void HandleGoldGambleClicked();
};