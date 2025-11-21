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

	/** * 몬스터를 죽인 플레이어의 상대방에게 몬스터를 보냅니다.
	 * @param KillerPlayerState 몬스터를 죽인 플레이어의 PS
	 * @param MonsterClassToSpawn 보낼 몬스터 클래스 (죽은 몬스터와 동일하거나 변형)
	 */
	void SendCounterAttackMonster(APlayerState* KillerPlayerState, TSubclassOf<AMonsterBaseCharacter> MonsterClassToSpawn);

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	virtual void BeginPlay() override;

	/** 플레이어가 성공적으로 로그인했을 때 서버에서 호출됩니다. (PIE, Listen, Dedicated 모두) */
	virtual void OnPostLogin(AController* NewPlayer) override;

private:
	void StartNextWave();
	void CheckGameOver();

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data")
	UDataTable* MonsterWaveDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Normal")
	int32 MonstersPerWave;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Normal")
	int32 GameoverMonsterNum;

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

	/** 스포너와 연결된 플레이어 컨트롤러를 찾아 반환하는 헬퍼 함수 */
	APlayerController* GetControllerForSpawner(AMonsterSpawner* Spawner) const;
};