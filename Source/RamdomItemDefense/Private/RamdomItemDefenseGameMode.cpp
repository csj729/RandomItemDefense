#include "RamdomItemDefenseGameMode.h"
#include "MyGameState.h"
#include "MyPlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "MonsterWaveData.h"
#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "RamdomItemDefensePlayerController.h"
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
		return; // GameState 없으면 함수 종료
	}

	// --- [코드 추가] 보스 타임아웃 체크 ---
	// 현재 웨이브가 1 이상이고(첫 웨이브 시작 시점 제외), 이전 웨이브가 보스 웨이브였는지 확인
	if (MyGameState->GetCurrentWave() > 0 && (MyGameState->GetCurrentWave() % 10 == 0))
	{
		// TODO: 현재 맵에 보스 몬스터가 아직 살아있는지 확인하는 로직 구현 필요
		// 예시: GameState에 보스 몬스터 액터 참조를 저장하고 IsValid 체크
		bool bBossIsAlive = false; // <<--- 이 부분을 실제 보스 생존 확인 로직으로 교체해야 합니다.
		// 예: TObjectPtr<AMonsterBaseCharacter> CurrentBoss; 를 GameState에 추가하고 관리

		if (bBossIsAlive)
		{
			// 보스가 아직 살아있다면 -> 보스 타임 아웃 게임오버
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("GAME OVER: Boss Time Out!"));

			// 모든 플레이어 컨트롤러를 가져와 게임오버 UI 표시 함수 호출
			for (APlayerState* PS : MyGameState->PlayerArray)
			{
				ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(PS->GetPlayerController());
				if (PC)
				{
					PC->ShowGameOverUI();
				}
			}
			// 다음 웨이브 진행 및 게임오버 체크 타이머 중지
			GetWorldTimerManager().ClearTimer(StageTimerHandle);
			GetWorldTimerManager().ClearTimer(GameOverCheckTimerHandle);
			return; // 게임오버 처리 후 함수 종료
		}
		// 보스가 죽었다면 정상적으로 다음 웨이브 진행
	}
	// --- 보스 타임아웃 체크 끝 ---


	// 다음 웨이브 번호 설정
	MyGameState->CurrentWave++;
	MyGameState->OnRep_CurrentWave(); // RepNotify 호출 (서버 UI 즉시 업데이트 및 클라이언트 복제)

	// 모든 플레이어에게 라운드 선택 횟수 2회 부여 (변경 없음)
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		if (MyPS)
		{
			// AddChoiceCount 함수 이름 확인 필요 (이전 코드에서는 AddChoiceCount로 되어 있었음)
			MyPS->AddChoiceCount(2); // 서버 전용 함수 호출
		}
	}

	// --- [코드 수정] 보스 웨이브 분기 로직 ---
	const int32 CurrentWave = MyGameState->GetCurrentWave();
	// 현재 웨이브가 1 이상이고 10의 배수이면 보스 웨이브로 간주
	const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);
	// 보스 웨이브는 120초, 일반 웨이브는 60초
	const float TimeLimitForThisWave = bIsBossWave ? BossStageTimeLimit : StageTimeLimit;

	// GameState에 이번 웨이브의 종료 시간 기록 (변경 없음)
	MyGameState->WaveEndTime = GetWorld()->GetTimeSeconds() + TimeLimitForThisWave;
	MyGameState->OnRep_WaveEndTime();

	// 보스 웨이브는 1마리, 일반 웨이브는 설정값(MonstersPerWave)만큼 스폰
	const int32 NumToSpawn = bIsBossWave ? 1 : MonstersPerWave;
	// --- 보스 웨이브 분기 끝 ---

	// 로그 출력 (변경 없음)
	if (GEngine)
	{
		FString WaveInfoMsg = FString::Printf(TEXT("Wave %d Info: bIsBoss=%s, TimeLimit=%.1f, NumToSpawn=%d"),
			CurrentWave, bIsBossWave ? TEXT("true") : TEXT("false"), TimeLimitForThisWave, NumToSpawn);
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, WaveInfoMsg);
	}

	// 몬스터 데이터 테이블 로드 및 스폰 명령 (변경 없음)
	if (MonsterWaveDataTable)
	{
		// 보스 웨이브와 일반 웨이브에 다른 Row Name 규칙이 필요할 수 있음 (예: "Boss1", "Boss2")
		// 여기서는 일단 "Wave10", "Wave20" 등으로 동일하게 사용
		FString RowName = FString::Printf(TEXT("Wave%d"), CurrentWave);
		FMonsterWaveData* WaveData = MonsterWaveDataTable->FindRow<FMonsterWaveData>(*RowName, TEXT(""));

		if (WaveData && WaveData->MonstersToSpawn.Num() > 0)
		{
			// TODO: 보스 웨이브일 경우 WaveData->MonstersToSpawn[0]이 실제 보스 몬스터 클래스인지 확인 필요
			TSubclassOf<AMonsterBaseCharacter> MonsterClass = WaveData->MonstersToSpawn[0];

			if (GEngine)
			{
				FString SpawnerCountMsg = FString::Printf(TEXT("Found %d Spawners. Issuing spawn command..."), MonsterSpawners.Num());
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, SpawnerCountMsg);
			}

			// 모든 스포너에게 스폰 명령 전달
			for (AMonsterSpawner* Spawner : MonsterSpawners)
			{
				if (Spawner)
				{
					// 스포너가 게임오버 상태가 아닐 때만 스폰하도록 추가 체크 가능
					// if (!Spawner->IsGameOver())
					// {
					Spawner->BeginSpawning(MonsterClass, NumToSpawn);
					// }
				}
			}

			// TODO: 보스 스폰 시 GameState 등에 보스 액터 참조 저장하는 로직 필요 (타임아웃 체크용)

		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("ERROR: WaveData for '%s' not found or no monsters assigned in DataTable!"), *RowName));
			// 웨이브 데이터가 없으면 다음 웨이브로 넘어가지 않도록 타이머 설정 중단 (선택 사항)
			// return;
		}
	}

	// 다음 웨이브 시작 타이머 설정 (변경 없음)
	GetWorld()->GetTimerManager().SetTimer(StageTimerHandle, this, &ARamdomItemDefenseGameMode::StartNextWave, TimeLimitForThisWave, false);
}

void ARamdomItemDefenseGameMode::CheckGameOver()
{
	// 등록된 모든 스포너를 순회하며 검사합니다.
	for (AMonsterSpawner* Spawner : MonsterSpawners)
	{
		// 스포너가 유효하고, 아직 게임오버 상태가 아니며, 몬스터 수가 60마리를 초과했는지 확인
		if (Spawner && !Spawner->IsGameOver() && Spawner->GetCurrentMonsterCount() > GameoverMonsterNum)
		{
			// 해당 스포너를 게임오버 상태로 만듭니다.
			Spawner->SetGameOver();

			APlayerController* Controller = GetControllerForSpawner(Spawner);
			ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(Controller);
			if (PC)
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("GAME OVER: Monster limit exceeded for %s"), *PC->GetName()));
				PC->ShowGameOverUI();
			}

			// TODO: 이 스포너와 연결된 플레이어에게 게임오버 UI를 띄우는 로직
		}
	}
}

/** 스포너 -> 컨트롤러 찾기 헬퍼 함수 */
APlayerController* ARamdomItemDefenseGameMode::GetControllerForSpawner(AMonsterSpawner* Spawner) const
{
	if (!Spawner) return nullptr;
	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState) return nullptr;

	// 모든 PlayerState를 순회
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		// PlayerState의 MySpawner가 주어진 Spawner와 일치하는지 확인
		if (MyPS && MyPS->MySpawner == Spawner)
		{
			// 일치하면 해당 PlayerState의 Controller 반환
			return MyPS->GetPlayerController();
		}
	}
	return nullptr; // 찾지 못함
}