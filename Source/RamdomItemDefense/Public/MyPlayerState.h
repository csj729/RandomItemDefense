#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ItemTypes.h"
#include "MyPlayerState.generated.h"

class AMonsterSpawner;
// --- [ �ڡڡ� �ڵ� �߰� �ڡڡ� ] ---
class AMyGameState; // AMyGameState ���� ����
// --- [ �ڵ� �߰� �� ] ---

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatLevelChangedDelegate, EItemStatType, StatType, int32, NewLevel);

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

	/** (���� ����) ��带 �Ҹ��մϴ�. ���� �� true ��ȯ */
	bool SpendGold(int32 Amount);

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

	// --- ���� ��ȭ ���� ---
public:
	/** (UI���� ȣ��) ������ ���� ��ȭ�� ��û�մϴ�. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Upgrade")
	void Server_RequestStatUpgrade(EItemStatType StatToUpgrade);

	/** Ư�� ������ ���� ��ȭ �ܰ踦 ��ȯ�մϴ�. */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetStatLevel(EItemStatType StatType) const;

	/** UI ���ε���: Ư�� ������ ������ ����� �� ȣ��˴ϴ�. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStatLevelChangedDelegate OnStatLevelChangedDelegate;

protected:
	/** �� ������ ���� ��ȭ �ܰ� (0���� ����) */
	UPROPERTY(ReplicatedUsing = OnRep_AttackDamageLevel)
	int32 AttackDamageLevel;
	UPROPERTY(ReplicatedUsing = OnRep_AttackSpeedLevel)
	int32 AttackSpeedLevel;
	UPROPERTY(ReplicatedUsing = OnRep_CritDamageLevel)
	int32 CritDamageLevel;
	UPROPERTY(ReplicatedUsing = OnRep_ArmorReductionLevel)
	int32 ArmorReductionLevel;
	UPROPERTY(ReplicatedUsing = OnRep_SkillActivationChanceLevel)
	int32 SkillActivationChanceLevel;

	/** Ŭ���̾�Ʈ���� StatLevels�� �����Ǿ��� �� ȣ��˴ϴ�. */
	UFUNCTION() void OnRep_AttackDamageLevel();
	UFUNCTION() void OnRep_AttackSpeedLevel();
	UFUNCTION() void OnRep_CritDamageLevel();
	UFUNCTION() void OnRep_ArmorReductionLevel();
	UFUNCTION() void OnRep_SkillActivationChanceLevel();

	/** (���� ����) ���� ���� ��ȭ�� �õ��ϰ� ����� ��ȯ�մϴ�. */
	bool TryUpgradeStat(EItemStatType StatToUpgrade);

protected:
	/** ���� ���� ���� ���� Ƚ�� */
	UPROPERTY(ReplicatedUsing = OnRep_ChoiceCount)
	int32 ChoiceCount;

	/** Ŭ���̾�Ʈ���� ChoiceCount�� �����Ǿ��� �� ȣ��˴ϴ�. */
	UFUNCTION()
	void OnRep_ChoiceCount();
	// ------------------------------------
};