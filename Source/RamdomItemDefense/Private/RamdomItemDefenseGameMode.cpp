#include "RamdomItemDefenseGameMode.h"
#include "MyGameState.h"
#include "MyPlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "MonsterWaveData.h"
#include "MonsterSpawner.h"
#include "GameFramework/PlayerState.h"
#include "Engine/Engine.h"

ARamdomItemDefenseGameMode::ARamdomItemDefenseGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	GameStateClass = AMyGameState::StaticClass();

	MonstersPerWave = 40;
	StageTimeLimit = 60.0f;
	BossStageTimeLimit = 120.0f;
}

void ARamdomItemDefenseGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("GameMode BeginPlay called. Game has started!"));
	}

	// --- [�ڵ� �̵�] ---
	// �����ʸ� ã�� ������ OnPostLogin���� �̵���ŵ�ϴ�.
	// (BeginPlay������ Ÿ�̸Ӹ� �����մϴ�)
	// ------------------

	FTimerHandle FirstWaveStartTimer;
	GetWorld()->GetTimerManager().SetTimer(FirstWaveStartTimer, this, &ARamdomItemDefenseGameMode::StartNextWave, 3.0f, false);
	GetWorld()->GetTimerManager().SetTimer(GameOverCheckTimerHandle, this, &ARamdomItemDefenseGameMode::CheckGameOver, 0.5f, true);
}

void ARamdomItemDefenseGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer); // �θ� �Լ��� ���� ȣ���մϴ�.

	// ����������, �׸��� ������ ����� ������� �� �� �� ���� �����մϴ�.
	if (HasAuthority() && MonsterSpawners.Num() == 0)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, TEXT("First player logged in. Finding Spawners..."));

		TArray<AActor*> FoundSpawners;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonsterSpawner::StaticClass(), FoundSpawners);

		for (AActor* SpawnerActor : FoundSpawners)
		{
			AMonsterSpawner* Spawner = Cast<AMonsterSpawner>(SpawnerActor);
			if (Spawner)
			{
				MonsterSpawners.Add(Spawner);
			}
		}

		if (GEngine)
		{
			FString SpawnerCountMsg = FString::Printf(TEXT("Dynamically found %d Spawners in the level."), MonsterSpawners.Num());
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Magenta, SpawnerCountMsg);
		}

		// (���� ����: �����ʸ� �̸������� �����Ͽ� �ε����� ������ų �� �ֽ��ϴ�)
		 MonsterSpawners.Sort([](const TObjectPtr<AMonsterSpawner>& A, const TObjectPtr<AMonsterSpawner>& B) {
		 	return A->GetName() < B->GetName();
		 });
	}

	if (NewPlayer == nullptr) return;

	// ���� ������ �÷��̾��� PlayerState�� �����ɴϴ�.
	AMyPlayerState* MyPS = NewPlayer->GetPlayerState<AMyPlayerState>();
	if (MyPS == nullptr) return;

	// GameState�� �����ɴϴ�. (OnPostLogin �������� GameState�� �׻� ��ȿ�մϴ�)
	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (MyGameState == nullptr) return;

	// GameState�� PlayerArray���� ��� ������ �÷��̾��� �ε���(����)�� ã���ϴ�.
	// (ȣ��Ʈ = 0, ù ��° Ŭ���̾�Ʈ = 1, ...)
	int32 PlayerIndex = MyGameState->PlayerArray.Find(MyPS);

	if (GEngine)
	{
		FString Msg = FString::Printf(TEXT("Player %s logged in. Assigned PlayerIndex: %d"), *MyPS->GetPlayerName(), PlayerIndex);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Msg);
	}

	// MonsterSpawners �迭�� �ش� �ε����� ��ȿ���� Ȯ���մϴ�.
	if (MonsterSpawners.IsValidIndex(PlayerIndex))
	{
		// �� �÷��̾��� PlayerState�� �ش� �ε����� �����ʸ� �Ҵ��մϴ�.
		MyPS->MySpawner = MonsterSpawners[PlayerIndex];

		// MySpawner ������ Replicated �����̹Ƿ�,
		// ������ �� ���� �����ϸ� �ڵ����� �ش� Ŭ���̾�Ʈ���� �����˴ϴ�.
		// Ŭ���̾�Ʈ������ OnRep_MySpawner�� ȣ��ǰ�, UI ���ε��� �Ͼ�ϴ�.

		if (GEngine)
		{
			FString SpawnerName = GetNameSafe(MyPS->MySpawner);
			FString Msg = FString::Printf(TEXT("Assigned Spawner '%s' to PlayerIndex %d"), *SpawnerName, PlayerIndex);
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, Msg);
		}
	}
	else
	{
		// �ʿ� ��ġ�� ������ ������ ���� �÷��̾ ������ ���
		if (GEngine)
		{
			FString Msg = FString::Printf(TEXT("ERROR: No valid spawner found at index %d for player %s."), PlayerIndex, *MyPS->GetPlayerName());
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Msg);
		}
	}
}

void ARamdomItemDefenseGameMode::StartNextWave()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("StartNextWave called."));

	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ERROR: GameState is NOT valid!"));
		return;
	}

	MyGameState->CurrentWave++;
	MyGameState->OnRep_CurrentWave();

	// ��� �÷��̾�� ���� ���� Ƚ�� 2ȸ�� �ο��մϴ�.
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		if (MyPS)
		{
			MyPS->AddChoiceCount(2); // ���� ���� �Լ� ȣ��
		}
	}

	const int32 CurrentWave = MyGameState->GetCurrentWave();
	const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);
	const float TimeLimitForThisWave = bIsBossWave ? BossStageTimeLimit : StageTimeLimit;

	// GameState�� �̹� ���̺��� ���� �ð��� ����մϴ�.
	MyGameState->WaveEndTime = GetWorld()->GetTimeSeconds() + TimeLimitForThisWave;
	MyGameState->OnRep_WaveEndTime();

	const int32 NumToSpawn = bIsBossWave ? 1 : MonstersPerWave;

	if (GEngine)
	{
		FString WaveInfoMsg = FString::Printf(TEXT("Wave %d Info: bIsBoss=%s, TimeLimit=%.1f, NumToSpawn=%d"),
			CurrentWave, bIsBossWave ? TEXT("true") : TEXT("false"), TimeLimitForThisWave, NumToSpawn);
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, WaveInfoMsg);
	}

	if (MonsterWaveDataTable)
	{
		FString RowName = FString::Printf(TEXT("Wave%d"), CurrentWave);
		FMonsterWaveData* WaveData = MonsterWaveDataTable->FindRow<FMonsterWaveData>(*RowName, TEXT(""));

		if (WaveData && WaveData->MonstersToSpawn.Num() > 0)
		{
			TSubclassOf<AMonsterBaseCharacter> MonsterClass = WaveData->MonstersToSpawn[0];

			if (GEngine)
			{
				FString SpawnerCountMsg = FString::Printf(TEXT("Found %d Spawners. Issuing spawn command..."), MonsterSpawners.Num());
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, SpawnerCountMsg);
			}

			for (AMonsterSpawner* Spawner : MonsterSpawners)
			{
				if (Spawner)
				{
					Spawner->BeginSpawning(MonsterClass, NumToSpawn);
				}
			}
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ERROR: WaveData not found or no monsters assigned in DataTable!"));
			return;
		}
	}

	GetWorld()->GetTimerManager().SetTimer(StageTimerHandle, this, &ARamdomItemDefenseGameMode::StartNextWave, TimeLimitForThisWave, false);
}

void ARamdomItemDefenseGameMode::CheckGameOver()
{
	// ��ϵ� ��� �����ʸ� ��ȸ�ϸ� �˻��մϴ�.
	for (AMonsterSpawner* Spawner : MonsterSpawners)
	{
		// �����ʰ� ��ȿ�ϰ�, ���� ���ӿ��� ���°� �ƴϸ�, ���� ���� 60������ �ʰ��ߴ��� Ȯ��
		if (Spawner && !Spawner->IsGameOver() && Spawner->GetCurrentMonsterCount() > 60)
		{
			// �ش� �����ʸ� ���ӿ��� ���·� ����ϴ�.
			Spawner->SetGameOver();

			// TODO: �� �����ʿ� ����� �÷��̾�� ���ӿ��� UI�� ���� ����
		}
	}
}