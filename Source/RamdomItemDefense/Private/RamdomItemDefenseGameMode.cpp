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
	PrimaryActorTick.bCanEverTick = true;
	bUseSeamlessTravel = true;

	GameStateClass = AMyGameState::StaticClass();

	MonstersPerWave = 40;
	StageTimeLimit = 60.0f;
	GameoverMonsterNum = 80;
	BossStageTimeLimit = 120.0f;
}

void ARamdomItemDefenseGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 게임이 시작되지 않았거나, GameState가 없으면 무시
	if (!bGameStarted) return;

	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState) return;

	// [핵심 로직] 현재 서버 시간이 웨이브 종료 시간을 지났는지 확인
	// bIsWaveInProgress 체크를 통해 중복 실행을 방지할 수도 있지만,
	// StartNextWave 내부에서 WaveEndTime을 미래로 밀어버리므로 자연스럽게 해결됩니다.
	float ServerTime = GetWorld()->GetTimeSeconds();

	if (ServerTime >= MyGameState->GetWaveEndTime())
	{
		// 시간이 다 되었으면 다음 웨이브 시작
		StartNextWave();
	}
}

void ARamdomItemDefenseGameMode::BeginPlay()
{
	Super::BeginPlay();

	RID_LOG(FColor::Yellow, TEXT("GameMode BeginPlay called."));

	GetWorld()->GetTimerManager().SetTimer(GameOverCheckTimerHandle, this, &ARamdomItemDefenseGameMode::CheckGameOver, 0.5f, true);
}

void ARamdomItemDefenseGameMode::OnPostLogin(AController* NewPlayer)	
{
	RID_LOG(FColor::Green, TEXT("PostLogin"));
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

	AssignSpawnerToPlayer(NewPlayer);

	// 4. [스폰 실행] 이제 부모 로직을 호출합니다. (내부에서 ChoosePlayerStart가 실행됨)
	Super::OnPostLogin(NewPlayer);

	CheckPlayerCountAndStart();
}

void ARamdomItemDefenseGameMode::CheckPlayerCountAndStart()
{
	if (bGameStarted) return;

	int32 CurrentPlayers = GetNumPlayers();

	// 모든 플레이어에게 대기 UI 띄우기
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(It->Get());
		if (PC)
		{
			// 준비가 덜 되었으므로 무조건 대기 UI 표시
			PC->Client_ShowWaitingUI();
		}
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
	}
}

void ARamdomItemDefenseGameMode::StartNextWave()
{
	// [수정] 로그 매크로 사용
	RID_LOG(FColor::Yellow, TEXT("StartNextWave called."));

	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState)
	{
		RID_LOG(FColor::Red, TEXT("ERROR: GameState is NOT valid!"));
		return;
	}

	// -----------------------------------------------------------------
	// 1. 보스 타임아웃 체크 (이전 웨이브가 보스전이었을 경우)
	// -----------------------------------------------------------------
	// CurrentWave가 증가하기 전이므로, 여기서 GetCurrentWave()는 '방금 끝난 웨이브'입니다.
	if (MyGameState->GetCurrentWave() > 0 && (MyGameState->GetCurrentWave() % 10 == 0))
	{
		// TODO: 실제 보스 생존 여부 확인 로직 필요
		// 현재는 예시로 false(보스 죽음) 처리되어 있습니다.
		bool bBossIsAlive = false;

		if (bBossIsAlive)
		{
			RID_LOG(FColor::Red, TEXT("GAME OVER: Boss Time Out!"));

			// 모든 플레이어에게 패배 UI 표시
			for (APlayerState* PS : MyGameState->PlayerArray)
			{
				ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(PS->GetPlayerController());
				if (PC)
				{
					PC->ShowGameOverUI();
				}
			}

			// 게임 종료 상태로 전환
			bGameStarted = false;
			// Tick에서의 감지를 막기 위해 웨이브 종료 시간을 무한대(혹은 과거)로 설정하거나 상태 플래그 변경
			// 여기서는 bGameStarted = false로 Tick이 멈추므로 충분합니다.

			GetWorldTimerManager().ClearTimer(GameOverCheckTimerHandle);
			return;
		}
	}

	// -----------------------------------------------------------------
	// 2. 웨이브 번호 갱신 및 플레이어 보상
	// -----------------------------------------------------------------
	MyGameState->CurrentWave++;
	MyGameState->OnRep_CurrentWave(); // 클라이언트 동기화

	// 플레이어들에게 선택권 부여 및 상태 리셋
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		if (MyPS)
		{
			MyPS->AddChoiceCount(2);
			MyPS->OnWaveStarted();
		}
	}

	// -----------------------------------------------------------------
	// 3. 다음 웨이브 시간 설정 (핵심: 타이머 대신 EndTime 설정)
	// -----------------------------------------------------------------
	const int32 CurrentWave = MyGameState->GetCurrentWave();
	const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);
	const float TimeLimitForThisWave = bIsBossWave ? BossStageTimeLimit : StageTimeLimit;

	// [중요] 현재 시간 + 웨이브 지속 시간을 더해 '종료 시점'을 설정합니다.
	// 이제 GameMode의 Tick 함수가 이 시간을 감시하다가, 시간이 되면 StartNextWave를 다시 호출할 것입니다.
	MyGameState->WaveEndTime = GetWorld()->GetTimeSeconds() + TimeLimitForThisWave;
	MyGameState->OnRep_WaveEndTime(); // 클라이언트 타이머 동기화

	// -----------------------------------------------------------------
	// 4. 몬스터 스폰
	// -----------------------------------------------------------------
	const int32 NumToSpawn = bIsBossWave ? 1 : MonstersPerWave;

	RID_LOG(FColor::White, TEXT("Wave %d Info: bIsBoss=%s, TimeLimit=%.1f, NumToSpawn=%d"),
		CurrentWave, bIsBossWave ? TEXT("true") : TEXT("false"), TimeLimitForThisWave, NumToSpawn);

	if (MonsterWaveDataTable)
	{
		FString RowName = FString::Printf(TEXT("Wave%d"), CurrentWave);
		FMonsterWaveData* WaveData = MonsterWaveDataTable->FindRow<FMonsterWaveData>(*RowName, TEXT(""));

		if (WaveData && WaveData->MonstersToSpawn.Num() > 0)
		{
			TSubclassOf<AMonsterBaseCharacter> MonsterClass = WaveData->MonstersToSpawn[0];

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
			RID_LOG(FColor::Red, TEXT("ERROR: WaveData for '%s' not found or no monsters assigned in DataTable!"), *RowName);
		}
	}

}

void ARamdomItemDefenseGameMode::CheckGameOver()
{
	// 게임이 시작되지 않았으면 로직을 수행하지 않음
	if (!bGameStarted) return;

	// [설정] 경고가 울리기 시작할 몬스터 수 (예: 80 - 10 = 70마리)
	const int32 WarningThreshold = GameoverMonsterNum - 10;

	for (AMonsterSpawner* Spawner : MonsterSpawners)
	{
		// 스포너가 유효하지 않으면 건너뜀
		if (!Spawner) continue;

		// 현재 몬스터 수 확인
		int32 CurrentCount = Spawner->GetCurrentMonsterCount();

		// 이 스포너의 주인(플레이어 컨트롤러) 찾기
		APlayerController* TargetPC = GetControllerForSpawner(Spawner);
		ARamdomItemDefensePlayerController* RIDPC = Cast<ARamdomItemDefensePlayerController>(TargetPC);

		// -----------------------------------------------------------------
		// 1. 게임 오버 조건 체크 (제한 수 초과)
		// -----------------------------------------------------------------
		if (!Spawner->IsGameOver() && CurrentCount > GameoverMonsterNum)
		{
			Spawner->SetGameOver();

			// [중요] 게임이 끝났으므로 경고음 끄기 (승리/패배 BGM과 겹치지 않게)
			if (RIDPC)
			{
				RIDPC->Client_SetWarningAlarm(false);
			}

			APlayerController* LoserPC = TargetPC;
			APlayerController* WinnerPC = nullptr;

			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (PC && PC != LoserPC)
				{
					WinnerPC = PC;
					break;
				}
			}

			// [패배 처리]
			if (LoserPC)
			{
				ARamdomItemDefensePlayerController* LoserRIDPC = Cast<ARamdomItemDefensePlayerController>(LoserPC);
				if (LoserRIDPC) LoserRIDPC->Client_ShowDefeatUI(); // 패배 UI 호출
			}

			// [승리 처리]
			if (WinnerPC)
			{
				ARamdomItemDefensePlayerController* WinnerRIDPC = Cast<ARamdomItemDefensePlayerController>(WinnerPC);
				if (WinnerRIDPC) WinnerRIDPC->Client_ShowVictoryUI(); // 승리 UI 호출
			}

			// 게임 종료 상태로 변경
			bGameStarted = false;
		}
		// 게임 오버는 아니지만, 플레이어 컨트롤러가 있고 게임 진행 중일 때
		else if (RIDPC && !Spawner->IsGameOver())
		{
			bool bShouldAlert = (CurrentCount >= WarningThreshold);

			RIDPC->Client_SetWarningAlarm(bShouldAlert);
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
		}
	}
}

AActor* ARamdomItemDefenseGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. PlayerState 확인
	AMyPlayerState* MyPS = Player ? Player->GetPlayerState<AMyPlayerState>() : nullptr;
	if (!MyPS) return Super::ChoosePlayerStart_Implementation(Player);

	// 2. [비상 할당 로직] 스포너가 없다면 직접 찾기
	if (!MyPS->MySpawner)
	{
		// (1) 호스트/클라이언트 구분 태그 설정
		FName TargetSpawnerTag = FName("Player1");
		if (!Player->IsLocalPlayerController())
		{
			TargetSpawnerTag = FName("Player2");
		}

		// (2) 월드에서 스포너 검색
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AMonsterSpawner::StaticClass(), TargetSpawnerTag, FoundActors);

		if (FoundActors.Num() > 0)
		{
			AMonsterSpawner* FoundSpawner = Cast<AMonsterSpawner>(FoundActors[0]);
			if (FoundSpawner)
			{
				// (3) 찾았다! 강제 할당
				MyPS->MySpawner = FoundSpawner;

				// 배열 인덱스 계산
				int32 Index = (TargetSpawnerTag == "Player1") ? 0 : 1;

				// 배열 크기가 작다면 강제로 늘려줍니다. (이게 핵심!)
				if (MonsterSpawners.Num() <= Index)
				{
					MonsterSpawners.SetNum(2); // 크기를 2로 설정 (나머지는 nullptr로 채워짐)
				}

				// 이제 안전하게 할당 가능
				if (MonsterSpawners.IsValidIndex(Index))
				{
					MonsterSpawners[Index] = FoundSpawner;
					RID_LOG(FColor::Green, TEXT("ChoosePlayerStart: [Emergency Array Fix] MonsterSpawners[%d] Updated."), Index);
				}
				// -------------------------------

				// UI 업데이트 알림
				MyPS->OnSpawnerAssignedDelegate.Broadcast(0);

				RID_LOG(FColor::Green, TEXT("ChoosePlayerStart: [Success] Assigned '%s' to %s"), *TargetSpawnerTag.ToString(), *Player->GetName());
			}
		}
		else
		{
			RID_LOG(FColor::Red, TEXT("ChoosePlayerStart: [Fail] No Spawner found with tag '%s'!"), *TargetSpawnerTag.ToString());
		}
	}

	// 3. PlayerStart 찾기 (기존 코드 유지)
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
			return FoundStarts[0];
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

UClass* ARamdomItemDefenseGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// 1. PlayerState 확인 (Seamless Travel 성공 시)
	if (AMyPlayerState* PS = InController->GetPlayerState<AMyPlayerState>())
	{
		if (PS->SelectedCharacterClass)
		{
			//RID_LOG(FColor::Green, TEXT("Spawn (PS): Found Class '%s'"), *GetNameSafe(PS->SelectedCharacterClass));
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
				//RID_LOG(FColor::Green, TEXT("Spawn (GI): Found Class '%s' from GameInstance"), *GetNameSafe(GI->SelectedCharacterClass));
				return GI->SelectedCharacterClass;
			}
		}
	}

	// 3. 실패 시 기본 캐릭터
	//RID_LOG(FColor::Yellow, TEXT("Spawn: Using Default Pawn"));
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ARamdomItemDefenseGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	RID_LOG(FColor::Cyan, TEXT("HandleSeamlessTravelPlayer: Player '%s' arrived via Seamless Travel."), *C->GetName());

	// [추가] 심리스 여행으로 넘어온 플레이어에게도 스포너 재할당 시도
	AssignSpawnerToPlayer(C);

	CheckPlayerCountAndStart();
}

// [신규 구현] 스포너 할당 함수
void ARamdomItemDefenseGameMode::AssignSpawnerToPlayer(AController* NewPlayer)
{
	AMyPlayerState* PS = NewPlayer->GetPlayerState<AMyPlayerState>();
	if (!PS) return;

	// 이미 스포너가 있다면 패스 (중복 할당 방지)
	if (PS->MySpawner) return;

	// 태그 결정 (Host = Player1, Client = Player2)
	// IsLocalPlayerController(): 서버 입장에서 로컬이면 호스트, 아니면 접속한 클라이언트
	FName TargetSpawnerTag = FName("Player1");
	if (!NewPlayer->IsLocalPlayerController())
	{
		TargetSpawnerTag = FName("Player2");
	}

	// 월드에서 해당 태그를 가진 스포너 찾기
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AMonsterSpawner::StaticClass(), TargetSpawnerTag, FoundActors);

	if (FoundActors.Num() > 0)
	{
		AMonsterSpawner* FoundSpawner = Cast<AMonsterSpawner>(FoundActors[0]);
		if (FoundSpawner)
		{
			// 1. PlayerState에 할당
			PS->MySpawner = FoundSpawner;

			// 2. 소유권 설정 (Replication을 위해 매우 중요!)
			FoundSpawner->SetOwner(NewPlayer);

			// 3. GameMode의 배열(MonsterSpawners) 관리
			int32 Index = (TargetSpawnerTag == "Player1") ? 0 : 1;

			// 배열 크기가 작다면 늘려줍니다.
			if (MonsterSpawners.Num() <= Index)
			{
				MonsterSpawners.SetNum(2);
			}

			if (MonsterSpawners.IsValidIndex(Index))
			{
				MonsterSpawners[Index] = FoundSpawner;
			}

			// 4. 델리게이트 호출 (서버측 로직 발동용)
			PS->OnSpawnerAssignedDelegate.Broadcast(0);

			UE_LOG(LogTemp, Log, TEXT("AssignSpawnerToPlayer: Assigned %s to %s"), *TargetSpawnerTag.ToString(), *NewPlayer->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AssignSpawnerToPlayer: Failed to find Spawner with tag %s"), *TargetSpawnerTag.ToString());
	}
}

void ARamdomItemDefenseGameMode::CheckReadyAndStart()
{
	if (bGameStarted) return;

	// 1. 최소 인원 확인 (2명 미만이면 시작 불가)
	if (GetNumPlayers() < 2) return;

	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState) return;

	// 2. 모든 플레이어가 준비되었는지 검사
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		if (!MyPS || !MyPS->IsReadyToPlay())
		{
			return;
		}
	}

	// 3. 모두 준비됨
	bGameStarted = true;
	RID_LOG(FColor::Green, TEXT("!!! ALL PLAYERS READY -> GAME START in 3 Seconds !!!"));

	// 대기 UI 끄기
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(It->Get());
		if (PC)
		{
			PC->Client_HideWaitingUI();
		}
	}

	// 3초 뒤 웨이브 시작 설정
	if (MyGameState)
	{
		MyGameState->MaxMonsterLimit = GameoverMonsterNum;
		MyGameState->WaveEndTime = GetWorld()->GetTimeSeconds() + 3.0f; // 모두 로딩 끝났으니 3초면 충분
		MyGameState->OnRep_WaveEndTime();
	}
}