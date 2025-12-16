#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ItemTypes.h"
#include "RamdomItemDefense.h" 
#include "MyPlayerState.generated.h"

class AMonsterSpawner;
class AMyGameState;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsReadyChangedDelegate, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatLevelChangedDelegate, EItemStatType, StatType, int32, NewLevel);

UCLASS()
class RAMDOMITEMDEFENSE_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMyPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	// =========================================================================
	//  Gold & Spawner
	// =========================================================================
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetGold() const { return Gold; }

	void AddGold(int32 Amount);
	bool SpendGold(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnGoldChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnSpawnerAssignedDelegate;

	UPROPERTY(ReplicatedUsing = OnRep_MySpawner, BlueprintReadOnly, Category = "Player State")
	TObjectPtr<AMonsterSpawner> MySpawner;

	// =========================================================================
	//  Lobby & Character Selection
	// =========================================================================
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Data")
	TSubclassOf<APawn> SelectedCharacterClass;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SetSelectedCharacter(TSubclassOf<APawn> NewClass);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SetPlayerName(const FString& NewName);

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsReady() const { return bIsReady; }

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_SetIsReady(bool bReady);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIsReadyChangedDelegate OnIsReadyChangedDelegate;

	// =========================================================================
	//  Game Logic: Choices & Rewards
	// =========================================================================
	// [Round Choice (Gacha)]
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetChoiceCount() const { return ChoiceCount; }
	void AddChoiceCount(int32 Count);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseRoundChoice(bool bChoseItemGacha);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnChoiceCountChangedDelegate;

	// [Common Item Choice]
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetCommonItemChoiceCount() const { return CommonItemChoiceCount; }
	void AddCommonItemChoice(int32 Count);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseCommonItemChoice(FName ChosenItemID);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnCommonItemChoiceCountChangedDelegate;

	// =========================================================================
	//  Stat Upgrades
	// =========================================================================
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Upgrade")
	void Server_RequestStatUpgrade(EItemStatType StatToUpgrade);

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetStatLevel(EItemStatType StatType) const;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStatLevelChangedDelegate OnStatLevelChangedDelegate;

	// =========================================================================
	//  Ultimate Skill
	// =========================================================================
	UFUNCTION(BlueprintPure, Category = "Player State|Ultimate")
	int32 GetUltimateCharge() const { return UltimateCharge; }

	UFUNCTION(BlueprintPure, Category = "Player State|Ultimate")
	int32 GetMaxUltimateCharge() const;

	void AddUltimateCharge(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Player State|Ultimate")
	void ResetUltimateCharge();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnUltimateChargeChangedDelegate;

	// =========================================================================
	//  Button Action (QTE)
	// =========================================================================
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnButtonActionLevelChangedDelegate;

	void OnWaveStarted();
	void TriggerButtonActionUI();
	void OnButtonActionTimeout();

	UFUNCTION(Server, Reliable)
	void Server_ReportButtonActionSuccess();

	UFUNCTION(Server, Reliable)
	void Server_ReportButtonActionFailure();

	UFUNCTION(Client, Reliable)
	void Client_NotifyButtonActionResult(bool bWasSuccess, int32 RewardIndex = -1);

	// =========================================================================
	//  Client Ready Check
	// =========================================================================
	UFUNCTION(Server, Reliable)
	void Server_SetReadyToPlay();

	bool IsReadyToPlay() const { return bIsReadyToPlay; }

protected:
	// --- [ Replication & Notify ] ---
	UPROPERTY(ReplicatedUsing = OnRep_Gold)
	int32 Gold;
	UFUNCTION() void OnRep_Gold();
	UFUNCTION() void OnRep_MySpawner();

	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsReady;
	UFUNCTION() void OnRep_IsReady();

	UPROPERTY(ReplicatedUsing = OnRep_ChoiceCount)
	int32 ChoiceCount;
	UFUNCTION() void OnRep_ChoiceCount();

	UPROPERTY(ReplicatedUsing = OnRep_CommonItemChoiceCount)
	int32 CommonItemChoiceCount;
	UFUNCTION() void OnRep_CommonItemChoiceCount();

	// --- [ Stat Levels ] ---
	UPROPERTY(ReplicatedUsing = OnRep_AttackDamageLevel) int32 AttackDamageLevel;
	UPROPERTY(ReplicatedUsing = OnRep_AttackSpeedLevel) int32 AttackSpeedLevel;
	UPROPERTY(ReplicatedUsing = OnRep_CritDamageLevel) int32 CritDamageLevel;
	UPROPERTY(ReplicatedUsing = OnRep_ArmorReductionLevel) int32 ArmorReductionLevel;
	UPROPERTY(ReplicatedUsing = OnRep_SkillActivationChanceLevel) int32 SkillActivationChanceLevel;

	UFUNCTION() void OnRep_AttackDamageLevel();
	UFUNCTION() void OnRep_AttackSpeedLevel();
	UFUNCTION() void OnRep_CritDamageLevel();
	UFUNCTION() void OnRep_ArmorReductionLevel();
	UFUNCTION() void OnRep_SkillActivationChanceLevel();

	bool TryUpgradeStat(EItemStatType StatToUpgrade);

	// --- [ Ultimate ] ---
	UPROPERTY(ReplicatedUsing = OnRep_UltimateCharge)
	int32 UltimateCharge;
	UFUNCTION() void OnRep_UltimateCharge();

	// --- [ Button Action ] ---
	UPROPERTY(ReplicatedUsing = OnRep_ButtonActionLevel)
	int32 ButtonActionLevel;
	UFUNCTION() void OnRep_ButtonActionLevel();

	UPROPERTY(Replicated)
	bool bIsButtonActionSequenceFinishedThisStage;

	FTimerHandle ButtonActionTimerHandle;
	FTimerHandle ButtonActionInputTimeoutHandle;

	TArray<float> ButtonActionTimingWindows = { 2.0f, 1.6f, 1.2f, 1.0f, 0.7f };
	EButtonActionKey CurrentRequiredButtonActionKey;
	bool bIsWaitingForButtonActionInput;

	UPROPERTY(EditDefaultsOnly, Category = "Button Action")
	TArray<TSubclassOf<UGameplayEffect>> ButtonActionRewardBuffs;

	// --- [ Client Ready Check ] ---
	bool bIsReadyToPlay = false;
};