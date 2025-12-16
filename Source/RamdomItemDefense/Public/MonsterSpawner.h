// Source/RamdomItemDefense/Public/MonsterSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemTypes.h"
#include "MonsterSpawner.generated.h"

// 전방 선언
class AMonsterBaseCharacter;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossStateChanged, bool, bIsBossAlive);

UCLASS()
class RAMDOMITEMDEFENSE_API AMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawner();

	// --- [ Override Functions ] ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- [ Delegates ] ---
	/** UI 업데이트를 위한 몬스터 수 변경 알림 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "MonsterSpawner|Event")
	FOnIntChangedDelegate OnMonsterCountChangedDelegate;

	// 보스 상태 변경 알림 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "MonsterSpawner|Event")
	FOnBossStateChanged OnBossStateChanged;

	// --- [ Public API : Spawning Control ] ---
	void BeginSpawning(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 Count);

	void SpawnCounterAttackMonster(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 MonsterWaveIndex);

	void OnMonsterKilled(bool bIsBoss);

	// --- [ Public API : Game State ] ---
	void SetGameOver();

	bool IsGameOver() const { return bIsGameOver; }

	int32 GetCurrentMonsterCount() const { return CurrentMonsterCount; }

	bool IsBossAlive() const { return ActiveBossCount > 0; }

	// --- [ Public Configuration ] ---
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "MonsterSpawner|Config")
	TObjectPtr<AActor> PatrolPathActor;

protected:
	// --- [ Lifecycle ] ---
	virtual void BeginPlay() override;

	// --- [ Replication Callback ] ---
	UFUNCTION()
	virtual void OnRep_CurrentMonsterCount();

	// --- [ GAS Configuration ] ---
	UPROPERTY(EditDefaultsOnly, Category = "MonsterSpawner|GAS")
	TSubclassOf<UGameplayEffect> MonsterStatInitEffect;

private:
	// --- [ Internal Spawning Logic ] ---
	void SpawnMonster();

	void InitSpawnedMonster(AMonsterBaseCharacter* SpawnedMonster, int32 WaveIndex);

	void ApplyMonsterStatsByWave(AMonsterBaseCharacter* Monster, int32 WaveIndex);

	// --- [ Internal State : Spawning ] ---
	UPROPERTY()
	TSubclassOf<AMonsterBaseCharacter> MonsterClassToSpawn;

	int32 TotalToSpawn;

	int32 SpawnCounter;

	FTimerHandle SpawnTimerHandle;

	// --- [ Internal State : Game Status ] ---
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentMonsterCount, Category = "MonsterSpawner|State")
	int32 CurrentMonsterCount;

	bool bIsGameOver;

	UPROPERTY(EditAnywhere, Category = "MonsterSpawner|Debug")
	bool bEnableDebug;

	int32 ActiveBossCount;
};