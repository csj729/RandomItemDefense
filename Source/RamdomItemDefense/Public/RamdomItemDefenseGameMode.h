#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RamdomItemDefenseGameMode.generated.h"

class UDataTable;
class AMonsterSpawner;
class AMonsterBaseCharacter;
class AMyGameState;

UCLASS(minimalapi)
class ARamdomItemDefenseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARamdomItemDefenseGameMode();

protected:
	virtual void BeginPlay() override;

private:
	void StartNextWave();
	void CheckGameOver();

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data")
	UDataTable* MonsterWaveDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Normal")
	int32 MonstersPerWave;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Normal")
	float StageTimeLimit;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Boss")
	float BossStageTimeLimit;

	// ================== [�ڵ� ����] ==================
	// ���� �����Ϳ��� ������ �ʿ䰡 �����Ƿ� UPROPERTY �����ڸ� �����մϴ�.
	// GameMode ���ο����� ����ϴ� ������ �˴ϴ�.
	TArray<TObjectPtr<AMonsterSpawner>> MonsterSpawners;
	// ===============================================

	FTimerHandle StageTimerHandle;
	FTimerHandle GameOverCheckTimerHandle;
};