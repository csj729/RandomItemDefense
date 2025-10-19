#include "MyGameState.h"
#include "Net/UnrealNetwork.h"

AMyGameState::AMyGameState()
{
    CurrentWave = 0; // 0으로 초기화
}

void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMyGameState, CurrentWave); // 복제 변수 등록
}

void AMyGameState::OnRep_CurrentWave()
{
    // ...
}