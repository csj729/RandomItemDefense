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
    DOREPLIFETIME(AMyGameState, CurrentWave);
    DOREPLIFETIME(AMyGameState, WaveEndTime);
    DOREPLIFETIME(AMyGameState, MaxMonsterLimit);

    DOREPLIFETIME(AMyGameState, MaxNormalStatLevel);
    DOREPLIFETIME(AMyGameState, MaxSpecialStatLevel);
    DOREPLIFETIME(AMyGameState, BaseLevelUpCost);
    DOREPLIFETIME(AMyGameState, IncreasingCostPerLevel);
    DOREPLIFETIME(AMyGameState, SpecialStatUpgradeChances);
}

void AMyGameState::OnRep_CurrentWave()
{
    OnWaveChangedDelegate.Broadcast(CurrentWave);
}

void AMyGameState::OnRep_WaveEndTime()
{
    OnWaveEndTimeChangedDelegate.Broadcast(WaveEndTime);
}