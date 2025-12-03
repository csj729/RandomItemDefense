#include "WaveTimerWidget.h"
#include "Components/TextBlock.h"
#include "MyGameState.h"
#include "Kismet/GameplayStatics.h"

void UWaveTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// GameState를 찾아 델리게이트에 바인딩합니다.
	MyGameState = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
	if (MyGameState)
	{
		// 1. 웨이브 변경 이벤트에 바인딩
		MyGameState->OnWaveChangedDelegate.AddDynamic(this, &UWaveTimerWidget::HandleWaveChanged);

		// 2. 현재 값으로 즉시 업데이트
		HandleWaveChanged(MyGameState->GetCurrentWave());
		UpdateTimerText();
	}
}

// 매 프레임 실행
void UWaveTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 틱마다 남은 시간 텍스트를 업데이트합니다.
	UpdateTimerText();
}

// GameState의 델리게이트가 호출할 함수
void UWaveTimerWidget::HandleWaveChanged(int32 NewWave)
{
	if (WaveText)
	{
		// "WAVE 10" 형식으로 텍스트 설정
		WaveText->SetText(FText::FromString(FString::Printf(TEXT("WAVE %d"), NewWave)));
	}
}

// 남은 시간을 계산하고 텍스트를 업데이트하는 함수
void UWaveTimerWidget::UpdateTimerText()
{
	if (!RemainTimeText || !MyGameState) return;

	// [수정 후] GameState가 제공하는 '서버 기준 시간'을 사용하여 오차 해결
	float Remaining = MyGameState->GetWaveEndTime() - MyGameState->GetServerWorldTimeSeconds();

	Remaining = FMath::Max(0.0f, Remaining); // 0초 이하로 내려가지 않도록 함

	// 분과 초로 변환
	int32 Minutes = FMath::FloorToInt(Remaining / 60.0f);
	int32 Seconds = FMath::FloorToInt(FMath::Fmod(Remaining, 60.0f));

	// "00:00" 형식으로 텍스트 설정
	RemainTimeText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}