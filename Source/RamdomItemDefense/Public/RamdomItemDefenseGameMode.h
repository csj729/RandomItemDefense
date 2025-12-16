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

	void SendCounterAttackMonster(APlayerState* KillerPlayerState, TSubclassOf<AMonsterBaseCharacter> MonsterClassToSpawn, int32 MonsterWaveIndex);
	void CheckReadyAndStart();

	// --- [ Overrides ] ---
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

protected:
	// --- [ Lifecycle ] ---
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- [ Player Management ] ---
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	void CheckPlayerCountAndStart();
	void AssignSpawnerToPlayer(AController* NewPlayer);

private:
	// --- [ Game Logic ] ---
	void StartNextWave();
	void CheckGameOver();

	APlayerController* GetControllerForSpawner(AMonsterSpawner* Spawner) const;

	// --- [ Config : Wave Data ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Wave Data")
	TObjectPtr<UDataTable> MonsterWaveDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Normal")
	int32 MonstersPerWave;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Normal")
	int32 GameoverMonsterNum;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Normal")
	float StageTimeLimit;

	UPROPERTY(EditDefaultsOnly, Category = "Wave Data|Boss")
	float BossStageTimeLimit;

	// --- [ Internal State ] ---
	TArray<TObjectPtr<AMonsterSpawner>> MonsterSpawners;
	FTimerHandle GameOverCheckTimerHandle;

	bool bIsWaveInProgress = false;
	bool bGameStarted = false;
};