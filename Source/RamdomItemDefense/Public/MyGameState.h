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

	// --- [ Override Functions ] ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- [ Public API : Wave & Time ] ---
	UFUNCTION(BlueprintPure, Category = "Wave")
	int32 GetCurrentWave() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category = "Wave")
	float GetWaveEndTime() const { return WaveEndTime; }

	// --- [ Delegates ] ---
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnWaveChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnFloatChangedDelegate OnWaveEndTimeChangedDelegate;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
	TArray<float> SpecialStatUpgradeChances = { 0.5f, 0.4f, 0.3f };

	// --- [ Upgrade Rules (Config) ] ---
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
	int32 MaxNormalStatLevel = 100;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
	int32 MaxSpecialStatLevel = 3;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
	int32 BaseLevelUpCost = 100;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules|Upgrade")
	int32 IncreasingCostPerLevel = 50;

	// --- [ Replication : Game Rules ] ---
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Rule")
	int32 MaxMonsterLimit;

protected:
	// --- [ Replication : Wave ] ---
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWave)
	int32 CurrentWave;

	UFUNCTION()
	void OnRep_CurrentWave();

	UPROPERTY(ReplicatedUsing = OnRep_WaveEndTime)
	float WaveEndTime;

	UFUNCTION()
	void OnRep_WaveEndTime();

	// GameMode 접근 허용
	friend class ARamdomItemDefenseGameMode;
};