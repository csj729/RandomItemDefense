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

    // ���� ���̺� ��ȣ�� �������� �Լ�
    UFUNCTION(BlueprintPure, Category = "Wave")
    int32 GetCurrentWave() const { return CurrentWave; }

protected:
    // �������� Ŭ���̾�Ʈ�� ������ �� ȣ��Ǵ� �Լ�
    UFUNCTION()
    void OnRep_CurrentWave();

    // ���� ���̺� ��ȣ. �������� �� ���� �ٲٸ� ��� Ŭ���̾�Ʈ���� �ڵ����� ����ȭ�˴ϴ�.
    UPROPERTY(ReplicatedUsing = OnRep_CurrentWave)
    int32 CurrentWave;

    // GameMode�� �� Ŭ������ private ������ ������ �� �ֵ��� ģ�� Ŭ������ ����
    friend class ARamdomItemDefenseGameMode;
};