#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ItemTypes.h"
#include "MyGameState.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API AMyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AMyGameState();

    // --- [코드 추가] ---
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    // ------------------

    // 현재 웨이브 번호를 가져오는 함수
    UFUNCTION(BlueprintPure, Category = "Wave")
    int32 GetCurrentWave() const { return CurrentWave; }

    // --- [코드 추가] ---
    // 현재 웨이브 종료 시간을 가져오는 함수
    UFUNCTION(BlueprintPure, Category = "Wave")
    float GetWaveEndTime() const { return WaveEndTime; }

    /** UI 바인딩용 델리게이트 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnIntChangedDelegate OnWaveChangedDelegate;

    /** UI 바인딩용 델리게이트 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFloatChangedDelegate OnWaveEndTimeChangedDelegate;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Rule")
    int32 MaxMonsterLimit;

    // --- [추가] 스탯 강화 규칙 (UI 표시를 위해 GameState에서 관리) ---
    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
    int32 MaxNormalStatLevel = 100;

    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
    int32 MaxSpecialStatLevel = 3;

    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
    int32 BaseLevelUpCost = 100;

    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
    int32 IncreasingCostPerLevel = 50;

    /** 특수 스탯 강화 성공 확률 (0레벨->1, 1->2, 2->3 ...) */
    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
    TArray<float> SpecialStatUpgradeChances = { 0.5f, 0.4f, 0.3f };

protected:
    // 서버에서 클라이언트로 복제될 때 호출되는 함수
    UFUNCTION()
    void OnRep_CurrentWave();

    // --- [코드 추가] ---
    UFUNCTION()
    void OnRep_WaveEndTime();
    // ------------------

    // 현재 웨이브 번호. 서버에서 이 값을 바꾸면 모든 클라이언트에게 자동으로 동기화됩니다.
    UPROPERTY(ReplicatedUsing = OnRep_CurrentWave)
    int32 CurrentWave;

    // --- [코드 추가] ---
    // 이번 웨이브가 끝나는 월드 시간 (서버가 설정)
    UPROPERTY(ReplicatedUsing = OnRep_WaveEndTime)
    float WaveEndTime;
    // ------------------

    // GameMode가 이 클래스의 private 변수에 접근할 수 있도록 친구 클래스로 선언
    friend class ARamdomItemDefenseGameMode;
};