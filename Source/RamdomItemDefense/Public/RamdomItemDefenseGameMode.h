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

	// ================== [코드 수정] ==================
	// 이제 에디터에서 설정할 필요가 없으므로 UPROPERTY 지정자를 제거합니다.
	// GameMode 내부에서만 사용하는 변수가 됩니다.
	TArray<TObjectPtr<AMonsterSpawner>> MonsterSpawners;
	// ===============================================

	FTimerHandle StageTimerHandle;
	FTimerHandle GameOverCheckTimerHandle;
};