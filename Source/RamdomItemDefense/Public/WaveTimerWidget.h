#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaveTimerWidget.generated.h"

class UTextBlock;
class AMyGameState;

UCLASS()
class RAMDOMITEMDEFENSE_API UWaveTimerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- [ Handlers ] ---
	UFUNCTION()
	void HandleWaveChanged(int32 NewWave);

	void UpdateTimerText();

	// --- [ UMG Binding ] ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> WaveText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RemainTimeText;

	UPROPERTY()
	TObjectPtr<AMyGameState> MyGameState;
};