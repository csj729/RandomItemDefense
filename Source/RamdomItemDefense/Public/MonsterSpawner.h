#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemTypes.h"
#include "MonsterSpawner.generated.h"

class AMonsterBaseCharacter;

UCLASS()
class RAMDOMITEMDEFENSE_API AMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawner();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** UI 바인딩용 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnMonsterCountChangedDelegate;

	// GameMode가 호출할 스폰 시작 함수
	void BeginSpawning(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 Count);

	// 몬스터가 죽었을 때 호출할 함수
	void OnMonsterKilled();

	// 현재 스폰된 몬스터 수를 반환하는 함수
	int32 GetCurrentMonsterCount() const { return CurrentMonsterCount; }

	// 이 스테이지를 게임오버 상태로 만드는 함수
	void SetGameOver();

	// 이 스테이지가 게임오버 상태인지 확인하는 함수
	bool IsGameOver() const { return bIsGameOver; }

	void SpawnCounterAttackMonster(TSubclassOf<AMonsterBaseCharacter> MonsterClass);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnRep_CurrentMonsterCount();

	UPROPERTY(EditDefaultsOnly, Category = "Spawning|GAS")
	TSubclassOf<UGameplayEffect> MonsterStatInitEffect;

private:
	// 몬스터를 실제로 스폰하는 함수
	void SpawnMonster();

	// 스폰할 몬스터의 클래스
	UPROPERTY()
	TSubclassOf<AMonsterBaseCharacter> MonsterClassToSpawn;
		
	// 이번 웨이브에 스폰할 총 몬스터 수
	int32 TotalToSpawn;

	// 지금까지 스폰한 몬스터 수
	int32 SpawnCounter;

	// 스폰 타이머 핸들
	FTimerHandle SpawnTimerHandle;

	// 디버깅 활성화 여부를 에디터에서 켜고 끌 수 있는 변수
	UPROPERTY(EditAnywhere, Category = "Debugging")
	bool bEnableDebug;

	// 이 스포너가 관리하는 현재 몬스터 수
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentMonsterCount, Category = "Live Count")
	int32 CurrentMonsterCount;

	// 이 스테이지의 게임오버 여부
	bool bIsGameOver;
};