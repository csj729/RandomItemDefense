#include "MyGameState.h"
#include "Net/UnrealNetwork.h"

AMyGameState::AMyGameState()
{
    CurrentWave = 0;
    WaveEndTime = 0.f;
}

void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMyGameState, CurrentWave); // 복제 변수 등록
    DOREPLIFETIME(AMyGameState, WaveEndTime); // --- [코드 추가]
}

void AMyGameState::OnRep_CurrentWave()
{
    // --- [코드 추가] ---
    // 웨이브가 변경되었음을 UI(WBP_WaveTimer)에 알립니다.
    OnWaveChangedDelegate.Broadcast(CurrentWave);
    // ------------------
}

// --- [코드 추가] ---
void AMyGameState::OnRep_WaveEndTime()
{
    // 웨이브 종료 시간이 변경되었음을 UI(WBP_WaveTimer)에 알립니다.
    OnWaveEndTimeChangedDelegate.Broadcast(WaveEndTime);
}
// ------------------