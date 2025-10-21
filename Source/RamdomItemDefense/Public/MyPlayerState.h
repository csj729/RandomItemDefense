#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ItemTypes.h"
#include "MyPlayerState.generated.h"

class AMonsterSpawner;

UCLASS()
class RAMDOMITEMDEFENSE_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMyPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- ��� ���� (������) ---
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetGold() const { return Gold; }
	void AddGold(int32 Amount); // ���� ����

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnGoldChangedDelegate;

	/** UI ���ε��� ��������Ʈ (�����ʰ� �Ҵ�/�����Ǿ��� �� ȣ��) */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnSpawnerAssignedDelegate; // (FOnIntChangedDelegate�� ��Ȱ��, ���� �ǹ� ����)

	/** �� �÷��̾�� �Ҵ�� ���� �������Դϴ�. */
	UPROPERTY(ReplicatedUsing = OnRep_MySpawner, BlueprintReadOnly, Category = "Player State")
	TObjectPtr<AMonsterSpawner> MySpawner;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Gold)
	int32 Gold;
	UFUNCTION()
	void OnRep_Gold();

	UFUNCTION()
	void OnRep_MySpawner();

	// --- [�ڵ� �߰�] ���� ���� ���� ---
public:
	/** @brief ���� ���� ���� ���� Ƚ���� ��ȯ�մϴ�. */
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetChoiceCount() const { return ChoiceCount; }

	/** @brief (���� ����) ���� ���� Ƚ���� �����մϴ�. */
	void AddChoiceCount(int32 Count);

	/** (UI���� ȣ��) ���� ����(������/���)�� ����մϴ�. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseRoundChoice(bool bChoseItemGacha);

	/** UI ���ε��� ��������Ʈ */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnChoiceCountChangedDelegate;

protected:
	/** ���� ���� ���� ���� Ƚ�� */
	UPROPERTY(ReplicatedUsing = OnRep_ChoiceCount)
	int32 ChoiceCount;

	/** Ŭ���̾�Ʈ���� ChoiceCount�� �����Ǿ��� �� ȣ��˴ϴ�. */
	UFUNCTION()
	void OnRep_ChoiceCount();
	// ------------------------------------
};