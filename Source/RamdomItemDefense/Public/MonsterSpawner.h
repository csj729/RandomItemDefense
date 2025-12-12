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
	/** 웨이브 시작 시 호출: 특정 몬스터를 n마리 스폰 시작 */
	void BeginSpawning(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 Count);

	/** 반격 몬스터(이벤트) 즉시 스폰 */
	void SpawnCounterAttackMonster(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 MonsterWaveIndex);

	/** 몬스터 사망 시 호출 (카운트 감소) */
	void OnMonsterKilled(bool bIsBoss);

	// --- [ Public API : Game State ] ---
	/** 게임 오버 처리 (스폰 중단) */
	void SetGameOver();

	/** 현재 게임 오버 상태인지 확인 */
	bool IsGameOver() const { return bIsGameOver; }

	/** 현재 살아있는 몬스터 수 반환 */
	int32 GetCurrentMonsterCount() const { return CurrentMonsterCount; }

	/** 현재 보스 몬스터가 살아있는지 확인 */
	bool IsBossAlive() const { return ActiveBossCount > 0; }

	// --- [ Public Configuration ] ---
	/** 몬스터가 따라갈 패트롤 경로 액터 (에디터 설정) */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "MonsterSpawner|Config")
	TObjectPtr<AActor> PatrolPathActor;

protected:
	// --- [ Lifecycle ] ---
	virtual void BeginPlay() override;

	// --- [ Replication Callback ] ---
	UFUNCTION()
	virtual void OnRep_CurrentMonsterCount();

	// --- [ GAS Configuration ] ---
	/** 몬스터 스폰 시 적용할 초기 스탯 GE (체력/방어력 보정) */
	UPROPERTY(EditDefaultsOnly, Category = "MonsterSpawner|GAS")
	TSubclassOf<UGameplayEffect> MonsterStatInitEffect;

private:
	// --- [ Internal Spawning Logic ] ---
	/** 타이머에 의해 호출되는 실제 스폰 함수 */
	void SpawnMonster();

	/** 스폰된 몬스터의 공통 초기화 로직 (스포너 설정, 스탯 적용, 카운트 증가) */
	void InitSpawnedMonster(AMonsterBaseCharacter* SpawnedMonster, int32 WaveIndex);

	/** 웨이브 레벨에 따른 몬스터 스탯(체력, 방어력, 외형) 적용 로직 */
	void ApplyMonsterStatsByWave(AMonsterBaseCharacter* Monster, int32 WaveIndex);

	// --- [ Internal State : Spawning ] ---
	/** 이번 웨이브에 스폰할 몬스터 클래스 */
	UPROPERTY()
	TSubclassOf<AMonsterBaseCharacter> MonsterClassToSpawn;

	/** 이번 웨이브 총 스폰 예정 수 */
	int32 TotalToSpawn;

	/** 현재까지 스폰한 수 */
	int32 SpawnCounter;

	/** 스폰 주기 관리 타이머 */
	FTimerHandle SpawnTimerHandle;

	// --- [ Internal State : Game Status ] ---
	/** 현재 살아있는 몬스터 수 (Replicated) */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentMonsterCount, Category = "MonsterSpawner|State")
	int32 CurrentMonsterCount;

	/** 게임 오버 플래그 */
	bool bIsGameOver;

	/** 디버그 모드 활성화 여부 */
	UPROPERTY(EditAnywhere, Category = "MonsterSpawner|Debug")
	bool bEnableDebug;

	int32 ActiveBossCount;
};