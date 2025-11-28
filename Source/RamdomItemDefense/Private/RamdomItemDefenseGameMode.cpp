// csj729/randomitemdefense/RandomItemDefense-78a128504f0127dc02646504d4a1e1c677a0e811/Source/RamdomItemDefense/Private/RamdomItemDefenseGameMode.cpp
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
#include "GameFramework/PlayerStart.h" 
#include "GameFramework/Controller.h"
#include "RIDGameInstance.h"
#include "Engine/Engine.h"
#include "RamdomItemDefense.h" // RID_LOG 매크로용

ARamdomItemDefenseGameMode::ARamdomItemDefenseGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseSeamlessTravel = true;

	GameStateClass = AMyGameState::StaticClass();

	MonstersPerWave = 40;
	StageTimeLimit = 60.0f;
	GameoverMonsterNum = 60;
	BossStageTimeLimit = 120.0f;
}

void ARamdomItemDefenseGameMode::BeginPlay()
{
	Super::BeginPlay();

	RID_LOG(FColor::Yellow, TEXT("GameMode BeginPlay called."));

	GetWorld()->GetTimerManager().SetTimer(GameOverCheckTimerHandle, this, &ARamdomItemDefenseGameMode::CheckGameOver, 0.5f, true);
}

void ARamdomItemDefenseGameMode::OnPostLogin(AController* NewPlayer)
{
	URIDGameInstance* RIDGI = Cast<URIDGameInstance>(GetGameInstance());
	if (RIDGI && !RIDGI->PlayerName.IsEmpty())
	{
		// 2. [이름 변경] PlayerState에 이름 적용
		if (NewPlayer->PlayerState)
		{
			NewPlayer->PlayerState->SetPlayerName(RIDGI->PlayerName);

			// 로그로 확인
			UE_LOG(LogTemp, Warning, TEXT("Player Name Updated to: %s"), *RIDGI->PlayerName);
		}
	}

	// 4. [스폰 실행] 이제 부모 로직을 호출합니다. (내부에서 ChoosePlayerStart가 실행됨)
	Super::OnPostLogin(NewPlayer);

	CheckPlayerCountAndStart();
}

void ARamdomItemDefenseGameMode::CheckPlayerCountAndStart()
{
	if (bGameStarted) return;

	int32 CurrentPlayers = GetNumPlayers();

	// 1. 현재 접속한 모든 플레이어에게 "대기 중" UI 띄우기 (아직 시작 안 했으므로)
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(It->Get());
		if (PC && CurrentPlayers < 2)
		{
			// 2명 미만이면 대기 UI 표시
			PC->Client_ShowWaitingUI();
		}
	}

	if (CurrentPlayers >= 2)
	{
		bGameStarted = true;
		RID_LOG(FColor::Green, TEXT("!!! GAME START !!!"));

		// 2. 게임 시작! 모든 플레이어의 대기 UI 끄기
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(It->Get());
			if (PC)
			{
				PC->Client_HideWaitingUI();
			}
		}

		// 3초 뒤 웨이브 시작
		FTimerHandle StartTimer;
		GetWorld()->GetTimerManager().SetTimer(StartTimer, this, &ARamdomItemDefenseGameMode::StartNextWave, 3.0f, false);
	}
}

void ARamdomItemDefenseGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (bGameStarted)
	{
		// 남은 사람 찾기
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* RemainingPC = It->Get();
			if (RemainingPC && RemainingPC != Exiting)
			{
				// 남은 사람 = 승리자
				ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(RemainingPC);
				if (PC)
				{
					PC->Client_ShowVictoryUI(); // 기권승 UI 표시
				}
			}
		}
		bGameStarted = false;
		GetWorldTimerManager().ClearTimer(StageTimerHandle);
	}
}

void ARamdomItemDefenseGameMode::StartNextWave()
{
	// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
	RID_LOG(FColor::Yellow, TEXT("StartNextWave called."));
	// -----------------------------------------

	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState)
	{
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		RID_LOG(FColor::Red, TEXT("ERROR: GameState is NOT valid!"));
		// -----------------------------------------
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
			// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
			RID_LOG(FColor::Red, TEXT("GAME OVER: Boss Time Out!"));
			// -----------------------------------------

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
			// 이 플레이어의 스킬 부스트 상태를 "새 웨이브"로 리셋
			MyPS->OnWaveStarted();
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
	// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
	// [수정] FString 변수를 만들지 않고 매크로에 직접 전달합니다.
	RID_LOG(FColor::White, TEXT("Wave %d Info: bIsBoss=%s, TimeLimit=%.1f, NumToSpawn=%d"),
		CurrentWave, bIsBossWave ? TEXT("true") : TEXT("false"), TimeLimitForThisWave, NumToSpawn);
	// -----------------------------------------

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

			// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
			// [수정] FString 변수를 만들지 않고 매크로에 직접 전달합니다.
			RID_LOG(FColor::Cyan, TEXT("Found %d Spawners. Issuing spawn command..."), MonsterSpawners.Num());
			// -----------------------------------------

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
			// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
			RID_LOG(FColor::Red, TEXT("ERROR: WaveData for '%s' not found or no monsters assigned in DataTable!"), *RowName);
			// -----------------------------------------
			// 웨이브 데이터가 없으면 다음 웨이브로 넘어가지 않도록 타이머 설정 중단 (선택 사항)
			// return;
		}
	}

	// 다음 웨이브 시작 타이머 설정 (변경 없음)
	GetWorld()->GetTimerManager().SetTimer(StageTimerHandle, this, &ARamdomItemDefenseGameMode::StartNextWave, TimeLimitForThisWave, false);
}

void ARamdomItemDefenseGameMode::CheckGameOver()
{
	if (!bGameStarted) return;

	for (AMonsterSpawner* Spawner : MonsterSpawners)
	{
		if (Spawner && !Spawner->IsGameOver() && Spawner->GetCurrentMonsterCount() > GameoverMonsterNum)
		{
			Spawner->SetGameOver();

			// 패배자 & 승리자 찾기
			APlayerController* LoserPC = GetControllerForSpawner(Spawner);
			APlayerController* WinnerPC = nullptr;

			// (2인 게임 기준) 패배자가 아닌 사람이 승리자
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (PC && PC != LoserPC)
				{
					WinnerPC = PC;
					break;
				}
			}

			// 승리자 이름 가져오기
			FString WinnerName = TEXT("Player");
			if (WinnerPC && WinnerPC->PlayerState)
			{
				WinnerName = WinnerPC->PlayerState->GetPlayerName();
			}

			// [패배 처리]
			if (LoserPC)
			{
				ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(LoserPC);
				if (PC) PC->Client_ShowDefeatUI(); // 패배 UI 호출
			}
				
			// [승리 처리]
			if (WinnerPC)
			{
				ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(WinnerPC);
				if (PC) PC->Client_ShowVictoryUI(); // 승리 UI 호출
			}

			// 게임 종료 처리
			GetWorldTimerManager().ClearTimer(StageTimerHandle);
			bGameStarted = false;
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

void ARamdomItemDefenseGameMode::SendCounterAttackMonster(APlayerState* KillerPlayerState, TSubclassOf<AMonsterBaseCharacter> MonsterClassToSpawn, int32 MonsterWaveIndex)
{
	if (!KillerPlayerState || !MonsterClassToSpawn) return;

	// 1. 킬러의 PlayerState로 변환
	AMyPlayerState* KillerPS = Cast<AMyPlayerState>(KillerPlayerState);
	if (!KillerPS || !KillerPS->MySpawner)
	{
		RID_LOG(FColor::Red, TEXT("PVP Error: Killer has no valid PlayerState or Spawner assigned!"));
		return;
	}

	// 2. 킬러의 스포너가 배열의 몇 번째에 있는지 확인
	// (MonsterSpawners[0]은 호스트용, [1]은 클라이언트용으로 고정되어 있음)
	int32 KillerSpawnerIndex = MonsterSpawners.Find(KillerPS->MySpawner);

	if (KillerSpawnerIndex == INDEX_NONE)
	{
		RID_LOG(FColor::Red, TEXT("PVP Error: Killer's Spawner is not in the GameMode list!"));
		return;
	}

	// 3. 상대방 인덱스 계산 (0이면 1, 1이면 0)
	int32 TargetIndex = (KillerSpawnerIndex + 1) % 2;

	// 4. 상대방 스포너에 스폰 명령
	if (MonsterSpawners.IsValidIndex(TargetIndex))
	{
		if (AMonsterSpawner* TargetSpawner = MonsterSpawners[TargetIndex])
		{
			// [수정] 웨이브 인덱스도 함께 전달
			TargetSpawner->SpawnCounterAttackMonster(MonsterClassToSpawn, MonsterWaveIndex);

			RID_LOG(FColor::Red, TEXT("PVP: Player(Spawner Index %d) sent monster(Wave %d) to Index %d!"), KillerSpawnerIndex, MonsterWaveIndex, TargetIndex);
		}
	}
}

AActor* ARamdomItemDefenseGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. 플레이어의 PlayerState를 가져와서, 할당된 스포너가 있는지 확인
	AMyPlayerState* MyPS = Player ? Player->GetPlayerState<AMyPlayerState>() : nullptr;
	if (!MyPS) return Super::ChoosePlayerStart_Implementation(Player);

	// 2. [핵심 수정] 스포너가 없다면? (타이밍 문제 or 할당 실패 시) -> 여기서 즉시 찾아서 연결!
	if (!MyPS->MySpawner)
	{
		RID_LOG(FColor::Yellow, TEXT("ChoosePlayerStart: MySpawner is NULL. Attempting Emergency Assignment..."));

		// (1) 호스트(Local)면 Player1, 클라이언트(Remote)면 Player2 태그 찾기
		FName TargetSpawnerTag = FName("Player1");
		if (!Player->IsLocalPlayerController())
		{
			TargetSpawnerTag = FName("Player2");
		}

		// (2) 월드에서 해당 태그를 가진 스포너 직접 검색
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AMonsterSpawner::StaticClass(), TargetSpawnerTag, FoundActors);

		if (FoundActors.Num() > 0)
		{
			AMonsterSpawner* FoundSpawner = Cast<AMonsterSpawner>(FoundActors[0]);
			if (FoundSpawner)
			{
				// (3) 찾았다! 강제 할당
				MyPS->MySpawner = FoundSpawner;

				// 스포너 배열도 갱신 (나중을 위해)
				int32 Index = (TargetSpawnerTag == "Player1") ? 0 : 1;
				if (MonsterSpawners.IsValidIndex(Index))
				{
					MonsterSpawners[Index] = FoundSpawner;
				}

				// UI 업데이트를 위해 알림 발송
				MyPS->OnSpawnerAssignedDelegate.Broadcast(0);

				RID_LOG(FColor::Green, TEXT("ChoosePlayerStart: [Emergency Success] Found & Assigned '%s' to %s"), *TargetSpawnerTag.ToString(), *Player->GetName());
			}
		}
		else
		{
			RID_LOG(FColor::Red, TEXT("ChoosePlayerStart: [Emergency Fail] CRITICAL! No Spawner found with tag '%s' in the level!"), *TargetSpawnerTag.ToString());
		}
	}

	// 3. 이제 스포너가 있을 것이므로(위에서 찾았으니), 해당 태그의 PlayerStart를 찾음
	FString PlayerStartTag = TEXT("");
	if (MyPS->MySpawner)
	{
		if (MyPS->MySpawner->ActorHasTag(FName("Player1")))
		{
			PlayerStartTag = TEXT("Player1");
		}
		else if (MyPS->MySpawner->ActorHasTag(FName("Player2")))
		{
			PlayerStartTag = TEXT("Player2");
		}
	}

	if (!PlayerStartTag.IsEmpty())
	{
		TArray<AActor*> FoundStarts;
		UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName(*PlayerStartTag), FoundStarts);

		if (FoundStarts.Num() > 0)
		{
			RID_LOG(FColor::Cyan, TEXT("ChoosePlayerStart: Matched Tag '%s' -> Spawning at '%s'"), *PlayerStartTag, *FoundStarts[0]->GetName());
			return FoundStarts[0]; // 찾은 스타트 지점 반환
		}
	}

	RID_LOG(FColor::Orange, TEXT("ChoosePlayerStart: TargetTag is Empty or Not Found. Using Random Spawn."));
	return Super::ChoosePlayerStart_Implementation(Player);
}

UClass* ARamdomItemDefenseGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// 1. PlayerState 확인 (Seamless Travel 성공 시)
	if (AMyPlayerState* PS = InController->GetPlayerState<AMyPlayerState>())
	{
		if (PS->SelectedCharacterClass)
		{
			RID_LOG(FColor::Green, TEXT("Spawn (PS): Found Class '%s'"), *GetNameSafe(PS->SelectedCharacterClass));
			return PS->SelectedCharacterClass;
		}
	}

	// 2. [추가] GameInstance 확인 (호스트용 안전장치)
	// 컨트롤러가 로컬(호스트)이라면 내 컴퓨터의 GameInstance를 읽어올 수 있습니다.
	if (InController->IsLocalController())
	{
		if (URIDGameInstance* GI = Cast<URIDGameInstance>(GetGameInstance()))
		{
			if (GI->SelectedCharacterClass)
			{
				RID_LOG(FColor::Green, TEXT("Spawn (GI): Found Class '%s' from GameInstance"), *GetNameSafe(GI->SelectedCharacterClass));
				return GI->SelectedCharacterClass;
			}
		}
	}

	// 3. 실패 시 기본 캐릭터
	RID_LOG(FColor::Yellow, TEXT("Spawn: Using Default Pawn"));
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}