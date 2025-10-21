#include "WaveTimerWidget.h"
#include "Components/TextBlock.h"
#include "MyGameState.h"
#include "Kismet/GameplayStatics.h"

void UWaveTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// GameState�� ã�� ��������Ʈ�� ���ε��մϴ�.
	MyGameState = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
	if (MyGameState)
	{
		// 1. ���̺� ���� �̺�Ʈ�� ���ε�
		MyGameState->OnWaveChangedDelegate.AddDynamic(this, &UWaveTimerWidget::HandleWaveChanged);

		// 2. ���� ������ ��� ������Ʈ
		HandleWaveChanged(MyGameState->GetCurrentWave());
		UpdateTimerText();
	}
}

// �� ������ ����
void UWaveTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ƽ���� ���� �ð� �ؽ�Ʈ�� ������Ʈ�մϴ�.
	UpdateTimerText();
}

// GameState�� ��������Ʈ�� ȣ���� �Լ�
void UWaveTimerWidget::HandleWaveChanged(int32 NewWave)
{
	if (WaveText)
	{
		// "WAVE 10" �������� �ؽ�Ʈ ����
		WaveText->SetText(FText::FromString(FString::Printf(TEXT("WAVE %d"), NewWave)));
	}
}

// ���� �ð��� ����ϰ� �ؽ�Ʈ�� ������Ʈ�ϴ� �Լ�
void UWaveTimerWidget::UpdateTimerText()
{
	if (!RemainTimeText || !MyGameState) return;

	// GameState�� ���� �ð����� ���� �ð��� ���� ���� �ð��� ����մϴ�.
	float Remaining = MyGameState->GetWaveEndTime() - GetWorld()->GetTimeSeconds();
	Remaining = FMath::Max(0.0f, Remaining); // 0�� ���Ϸ� �������� �ʵ��� ��

	// �а� �ʷ� ��ȯ
	int32 Minutes = FMath::FloorToInt(Remaining / 60.0f);
	int32 Seconds = FMath::FloorToInt(FMath::Fmod(Remaining, 60.0f));

	// "00:00" �������� �ؽ�Ʈ ����
	RemainTimeText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}