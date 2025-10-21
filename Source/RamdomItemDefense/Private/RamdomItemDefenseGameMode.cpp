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

	// --- [코드 이동] ---
	// 스포너를 찾는 로직을 OnPostLogin으로 이동시킵니다.
	// (BeginPlay에서는 타이머만 설정합니다)
	// ------------------

	FTimerHandle FirstWaveStartTimer;
	GetWorld()->GetTimerManager().SetTimer(FirstWaveStartTimer, this, &ARamdomItemDefenseGameMode::StartNextWave, 3.0f, false);
	GetWorld()->GetTimerManager().SetTimer(GameOverCheckTimerHandle, this, &ARamdomItemDefenseGameMode::CheckGameOver, 0.5f, true);
}

void ARamdomItemDefenseGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer); // 부모 함수를 먼저 호출합니다.

	// 서버에서만, 그리고 스포너 목록이 비어있을 때 딱 한 번만 실행합니다.
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

		// (선택 사항: 스포너를 이름순으로 정렬하여 인덱스를 고정시킬 수 있습니다)
		 MonsterSpawners.Sort([](const TObjectPtr<AMonsterSpawner>& A, const TObjectPtr<AMonsterSpawner>& B) {
		 	return A->GetName() < B->GetName();
		 });
	}

	if (NewPlayer == nullptr) return;

	// 새로 접속한 플레이어의 PlayerState를 가져옵니다.
	AMyPlayerState* MyPS = NewPlayer->GetPlayerState<AMyPlayerState>();
	if (MyPS == nullptr) return;

	// GameState를 가져옵니다. (OnPostLogin 시점에는 GameState가 항상 유효합니다)
	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (MyGameState == nullptr) return;

	// GameState의 PlayerArray에서 방금 접속한 플레이어의 인덱스(순서)를 찾습니다.
	// (호스트 = 0, 첫 번째 클라이언트 = 1, ...)
	int32 PlayerIndex = MyGameState->PlayerArray.Find(MyPS);

	if (GEngine)
	{
		FString Msg = FString::Printf(TEXT("Player %s logged in. Assigned PlayerIndex: %d"), *MyPS->GetPlayerName(), PlayerIndex);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Msg);
	}

	// MonsterSpawners 배열에 해당 인덱스가 유효한지 확인합니다.
	if (MonsterSpawners.IsValidIndex(PlayerIndex))
	{
		// 이 플레이어의 PlayerState에 해당 인덱스의 스포너를 할당합니다.
		MyPS->MySpawner = MonsterSpawners[PlayerIndex];

		// MySpawner 변수는 Replicated 변수이므로,
		// 서버가 이 값을 설정하면 자동으로 해당 클라이언트에게 복제됩니다.
		// 클라이언트에서는 OnRep_MySpawner가 호출되고, UI 바인딩이 일어납니다.

		if (GEngine)
		{
			FString SpawnerName = GetNameSafe(MyPS->MySpawner);
			FString Msg = FString::Printf(TEXT("Assigned Spawner '%s' to PlayerIndex %d"), *SpawnerName, PlayerIndex);
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, Msg);
		}
	}
	else
	{
		// 맵에 배치된 스포너 수보다 많은 플레이어가 접속한 경우
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

	// 모든 플레이어에게 라운드 선택 횟수 2회를 부여합니다.
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		if (MyPS)
		{
			MyPS->AddChoiceCount(2); // 서버 전용 함수 호출
		}
	}

	const int32 CurrentWave = MyGameState->GetCurrentWave();
	const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);
	const float TimeLimitForThisWave = bIsBossWave ? BossStageTimeLimit : StageTimeLimit;

	// GameState에 이번 웨이브의 종료 시간을 기록합니다.
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
	// 등록된 모든 스포너를 순회하며 검사합니다.
	for (AMonsterSpawner* Spawner : MonsterSpawners)
	{
		// 스포너가 유효하고, 아직 게임오버 상태가 아니며, 몬스터 수가 60마리를 초과했는지 확인
		if (Spawner && !Spawner->IsGameOver() && Spawner->GetCurrentMonsterCount() > 60)
		{
			// 해당 스포너를 게임오버 상태로 만듭니다.
			Spawner->SetGameOver();

			// TODO: 이 스포너와 연결된 플레이어에게 게임오버 UI를 띄우는 로직
		}
	}
}