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
    DOREPLIFETIME(AMyGameState, CurrentWave); // ���� ���� ���
    DOREPLIFETIME(AMyGameState, WaveEndTime); // --- [�ڵ� �߰�]
}

void AMyGameState::OnRep_CurrentWave()
{
    // --- [�ڵ� �߰�] ---
    // ���̺갡 ����Ǿ����� UI(WBP_WaveTimer)�� �˸��ϴ�.
    OnWaveChangedDelegate.Broadcast(CurrentWave);
    // ------------------
}

// --- [�ڵ� �߰�] ---
void AMyGameState::OnRep_WaveEndTime()
{
    // ���̺� ���� �ð��� ����Ǿ����� UI(WBP_WaveTimer)�� �˸��ϴ�.
    OnWaveEndTimeChangedDelegate.Broadcast(WaveEndTime);
}
// ------------------