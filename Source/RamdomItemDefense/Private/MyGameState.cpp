#include "MyGameState.h"
#include "Net/UnrealNetwork.h"

AMyGameState::AMyGameState()
{
    CurrentWave = 0; // 0���� �ʱ�ȭ
}

void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMyGameState, CurrentWave); // ���� ���� ���
}

void AMyGameState::OnRep_CurrentWave()
{
    // ...
}