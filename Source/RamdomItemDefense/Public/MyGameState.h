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

    // --- [�ڵ� �߰�] ---
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    // ------------------

    // ���� ���̺� ��ȣ�� �������� �Լ�
    UFUNCTION(BlueprintPure, Category = "Wave")
    int32 GetCurrentWave() const { return CurrentWave; }

    // --- [�ڵ� �߰�] ---
    // ���� ���̺� ���� �ð��� �������� �Լ�
    UFUNCTION(BlueprintPure, Category = "Wave")
    float GetWaveEndTime() const { return WaveEndTime; }

    /** UI ���ε��� ��������Ʈ */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnIntChangedDelegate OnWaveChangedDelegate;

    /** UI ���ε��� ��������Ʈ */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFloatChangedDelegate OnWaveEndTimeChangedDelegate;
    // ------------------

protected:
    // �������� Ŭ���̾�Ʈ�� ������ �� ȣ��Ǵ� �Լ�
    UFUNCTION()
    void OnRep_CurrentWave();

    // --- [�ڵ� �߰�] ---
    UFUNCTION()
    void OnRep_WaveEndTime();
    // ------------------

    // ���� ���̺� ��ȣ. �������� �� ���� �ٲٸ� ��� Ŭ���̾�Ʈ���� �ڵ����� ����ȭ�˴ϴ�.
    UPROPERTY(ReplicatedUsing = OnRep_CurrentWave)
    int32 CurrentWave;

    // --- [�ڵ� �߰�] ---
    // �̹� ���̺갡 ������ ���� �ð� (������ ����)
    UPROPERTY(ReplicatedUsing = OnRep_WaveEndTime)
    float WaveEndTime;
    // ------------------

    // GameMode�� �� Ŭ������ private ������ ������ �� �ֵ��� ģ�� Ŭ������ ����
    friend class ARamdomItemDefenseGameMode;
};