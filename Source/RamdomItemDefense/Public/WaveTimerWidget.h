#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaveTimerWidget.generated.h"

class UTextBlock; // TextBlock을 사용하기 위해 전방 선언
class AMyGameState;

UCLASS()
class RAMDOMITEMDEFENSE_API UWaveTimerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 매 프레임(Tick)마다 호출되어 남은 시간을 갱신합니다.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// GameState의 웨이브 변경 이벤트를 처리할 함수
	UFUNCTION()
	void HandleWaveChanged(int32 NewWave);

	// 남은 시간 텍스트를 업데이트하는 헬퍼 함수
	void UpdateTimerText();

protected:
	// UMG 디자이너에서 'WaveText'라는 이름의 TextBlock 위젯을 찾아 바인딩합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WaveText;

	// UMG 디자이너에서 'RemainTimeText'라는 이름의 TextBlock 위젯을 찾아 바인딩합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RemainTimeText;

	// GameState 참조
	UPROPERTY()
	TObjectPtr<AMyGameState> MyGameState;
};