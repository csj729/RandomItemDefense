#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MyGameState.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API AMyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AMyGameState();

    // 현재 웨이브 번호를 가져오는 함수
    UFUNCTION(BlueprintPure, Category = "Wave")
    int32 GetCurrentWave() const { return CurrentWave; }

protected:
    // 서버에서 클라이언트로 복제될 때 호출되는 함수
    UFUNCTION()
    void OnRep_CurrentWave();

    // 현재 웨이브 번호. 서버에서 이 값을 바꾸면 모든 클라이언트에게 자동으로 동기화됩니다.
    UPROPERTY(ReplicatedUsing = OnRep_CurrentWave)
    int32 CurrentWave;

    // GameMode가 이 클래스의 private 변수에 접근할 수 있도록 친구 클래스로 선언
    friend class ARamdomItemDefenseGameMode;
};