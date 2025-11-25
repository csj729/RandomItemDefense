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
#include "Engine/Engine.h"
#include "RamdomItemDefense.h" // RID_LOG 매크로용

ARamdomItemDefenseGameMode::ARamdomItemDefenseGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

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

	// [수정] BeginPlay에서도 스포너를 찾습니다. (혹시 OnPostLogin보다 늦게 실행될 경우를 대비)
	if (HasAuthority() && MonsterSpawners.Num() == 0)
	{
		// 로직을 함수화하거나 중복 코드를 작성합니다. 여기서는 간단히 블록 내부에서 처리합니다.
		MonsterSpawners.Init(nullptr, 2);
		TArray<AActor*> FoundSpawners;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonsterSpawner::StaticClass(), FoundSpawners);

		for (AActor* Actor : FoundSpawners)
		{
			AMonsterSpawner* Spawner = Cast<AMonsterSpawner>(Actor);
			if (Spawner)
			{
				if (Spawner->ActorHasTag(FName("Player1"))) MonsterSpawners[0] = Spawner;
				else if (Spawner->ActorHasTag(FName("Player2"))) MonsterSpawners[1] = Spawner;
			}
		}
	}

	FTimerHandle FirstWaveStartTimer;
	GetWorld()->GetTimerManager().SetTimer(FirstWaveStartTimer, this, &ARamdomItemDefenseGameMode::StartNextWave, 3.0f, false);
	GetWorld()->GetTimerManager().SetTimer(GameOverCheckTimerHandle, this, &ARamdomItemDefenseGameMode::CheckGameOver, 0.5f, true);
}

void ARamdomItemDefenseGameMode::OnPostLogin(AController* NewPlayer)
{
	// 1. [안전 장치] 스포너 리스트가 비어있다면 즉시 채워넣기 (호스트 접속 시점 대비)
	if (HasAuthority() && MonsterSpawners.Num() == 0)
	{
		MonsterSpawners.Init(nullptr, 2);
		TArray<AActor*> FoundSpawners;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonsterSpawner::StaticClass(), FoundSpawners);

		for (AActor* Actor : FoundSpawners)
		{
			AMonsterSpawner* Spawner = Cast<AMonsterSpawner>(Actor);
			if (Spawner)
			{
				if (Spawner->ActorHasTag(FName("Player1"))) MonsterSpawners[0] = Spawner;
				else if (Spawner->ActorHasTag(FName("Player2"))) MonsterSpawners[1] = Spawner;
			}
		}
	}

	// 2. [핵심] GameState의 PlayerArray를 이용해 내 순서(Index) 확인
	// (PlayerArray에는 접속한 플레이어들이 순서대로 들어옵니다. 0=호스트, 1=첫 접속자)
	int32 MyIndex = 0;
	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	AMyPlayerState* MyPS = NewPlayer->GetPlayerState<AMyPlayerState>();

	if (MyGameState && MyPS)
	{
		// 현재 리스트에 내가 없다면(아직 추가 전이라면) 맨 뒤 번호로 가정
		MyIndex = MyGameState->PlayerArray.Find(MyPS);
		if (MyIndex == INDEX_NONE)
		{
			MyIndex = MyGameState->PlayerArray.Num();
		}
	}

	// 2인 게임이므로 0, 1 인덱스만 유효하도록 보정
	MyIndex = MyIndex % 2;

	// 3. [선 할당] 부모의 OnPostLogin(스폰) 호출 전에, 스포너를 먼저 쥐어줍니다.
	if (MyPS && MonsterSpawners.IsValidIndex(MyIndex) && MonsterSpawners[MyIndex] != nullptr)
	{
		MyPS->MySpawner = MonsterSpawners[MyIndex];
		RID_LOG(FColor::Green, TEXT(">>> [Pre-Assign] Player Index %d assigned to Spawner '%s'"), MyIndex, *GetNameSafe(MyPS->MySpawner));
	}
	else
	{
		RID_LOG(FColor::Red, TEXT(">>> [Error] Could not assign spawner for Player Index %d"), MyIndex);
	}

	// 4. [스폰 실행] 이제 부모 로직을 호출합니다. (내부에서 ChoosePlayerStart가 실행됨)
	Super::OnPostLogin(NewPlayer);
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
				// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
				RID_LOG(FColor::Red, TEXT("GAME OVER: Monster limit exceeded for %s"), *PC->GetName());
				// -----------------------------------------
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

	FString TargetTag = TEXT("");

	// 2. 내 스포너가 있다면, 그 스포너의 태그("Player1" or "Player2")를 그대로 사용
	if (MyPS && MyPS->MySpawner)
	{
		if (MyPS->MySpawner->ActorHasTag(FName("Player1")))
		{
			TargetTag = TEXT("Player1");
		}
		else if (MyPS->MySpawner->ActorHasTag(FName("Player2")))
		{
			TargetTag = TEXT("Player2");
		}
	}

	// 3. 해당 태그를 가진 PlayerStart 찾기
	if (!TargetTag.IsEmpty())
	{
		TArray<AActor*> FoundStarts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

		for (AActor* Start : FoundStarts)
		{
			if (Start && Start->ActorHasTag(FName(*TargetTag)))
			{
				RID_LOG(FColor::Cyan, TEXT("ChoosePlayerStart: Matched Tag '%s' -> Start '%s'"), *TargetTag, *Start->GetName());
				return Start; // 찾았으면 즉시 반환
			}
		}
	}

	// 4. 못 찾았거나(에러 상황) 태그가 없다면 기본 로직 사용
	return Super::ChoosePlayerStart_Implementation(Player);
}