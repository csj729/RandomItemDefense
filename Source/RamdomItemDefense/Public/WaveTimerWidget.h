#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaveTimerWidget.generated.h"

class UTextBlock; // TextBlock�� ����ϱ� ���� ���� ����
class AMyGameState;

UCLASS()
class RAMDOMITEMDEFENSE_API UWaveTimerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// �� ������(Tick)���� ȣ��Ǿ� ���� �ð��� �����մϴ�.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// GameState�� ���̺� ���� �̺�Ʈ�� ó���� �Լ�
	UFUNCTION()
	void HandleWaveChanged(int32 NewWave);

	// ���� �ð� �ؽ�Ʈ�� ������Ʈ�ϴ� ���� �Լ�
	void UpdateTimerText();

protected:
	// UMG �����̳ʿ��� 'WaveText'��� �̸��� TextBlock ������ ã�� ���ε��մϴ�.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WaveText;

	// UMG �����̳ʿ��� 'RemainTimeText'��� �̸��� TextBlock ������ ã�� ���ε��մϴ�.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RemainTimeText;

	// GameState ����
	UPROPERTY()
	TObjectPtr<AMyGameState> MyGameState;
};