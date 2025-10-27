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
#include "Engine/Engine.h"
#include "RamdomItemDefense.h" // RID_LOG ��ũ�ο�

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

	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	RID_LOG(FColor::Yellow, TEXT("GameMode BeginPlay called. Game has started!"));
	// -----------------------------------------

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
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Magenta, TEXT("First player logged in. Finding Spawners..."));
		// -----------------------------------------

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

		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
		RID_LOG(FColor::Magenta, TEXT("Dynamically found %d Spawners in the level."), MonsterSpawners.Num());
		// -----------------------------------------

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

	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
	RID_LOG(FColor::Cyan, TEXT("Player %s logged in. Assigned PlayerIndex: %d"), *MyPS->GetPlayerName(), PlayerIndex);
	// -----------------------------------------

	// MonsterSpawners �迭�� �ش� �ε����� ��ȿ���� Ȯ���մϴ�.
	if (MonsterSpawners.IsValidIndex(PlayerIndex))
	{
		// �� �÷��̾��� PlayerState�� �ش� �ε����� �����ʸ� �Ҵ��մϴ�.
		MyPS->MySpawner = MonsterSpawners[PlayerIndex];

		// MySpawner ������ Replicated �����̹Ƿ�,
		// ������ �� ���� �����ϸ� �ڵ����� �ش� Ŭ���̾�Ʈ���� �����˴ϴ�.
		// Ŭ���̾�Ʈ������ OnRep_MySpawner�� ȣ��ǰ�, UI ���ε��� �Ͼ�ϴ�.

		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
		FString SpawnerName = GetNameSafe(MyPS->MySpawner); // FString ����� ���� OK
		RID_LOG(FColor::Green, TEXT("Assigned Spawner '%s' to PlayerIndex %d"), *SpawnerName, PlayerIndex);
		// -----------------------------------------
	}
	else
	{
		// �ʿ� ��ġ�� ������ ������ ���� �÷��̾ ������ ���
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
		RID_LOG(FColor::Red, TEXT("ERROR: No valid spawner found at index %d for player %s."), PlayerIndex, *MyPS->GetPlayerName());
		// -----------------------------------------
	}
}

void ARamdomItemDefenseGameMode::StartNextWave()
{
	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	RID_LOG(FColor::Yellow, TEXT("StartNextWave called."));
	// -----------------------------------------

	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("ERROR: GameState is NOT valid!"));
		// -----------------------------------------
		return; // GameState ������ �Լ� ����
	}

	// --- [�ڵ� �߰�] ���� Ÿ�Ӿƿ� üũ ---
	// ���� ���̺갡 1 �̻��̰�(ù ���̺� ���� ���� ����), ���� ���̺갡 ���� ���̺꿴���� Ȯ��
	if (MyGameState->GetCurrentWave() > 0 && (MyGameState->GetCurrentWave() % 10 == 0))
	{
		// TODO: ���� �ʿ� ���� ���Ͱ� ���� ����ִ��� Ȯ���ϴ� ���� ���� �ʿ�
		// ����: GameState�� ���� ���� ���� ������ �����ϰ� IsValid üũ
		bool bBossIsAlive = false; // <<--- �� �κ��� ���� ���� ���� Ȯ�� �������� ��ü�ؾ� �մϴ�.
		// ��: TObjectPtr<AMonsterBaseCharacter> CurrentBoss; �� GameState�� �߰��ϰ� ����

		if (bBossIsAlive)
		{
			// ������ ���� ����ִٸ� -> ���� Ÿ�� �ƿ� ���ӿ���
			// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
			RID_LOG(FColor::Red, TEXT("GAME OVER: Boss Time Out!"));
			// -----------------------------------------

			// ��� �÷��̾� ��Ʈ�ѷ��� ������ ���ӿ��� UI ǥ�� �Լ� ȣ��
			for (APlayerState* PS : MyGameState->PlayerArray)
			{
				ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(PS->GetPlayerController());
				if (PC)
				{
					PC->ShowGameOverUI();
				}
			}
			// ���� ���̺� ���� �� ���ӿ��� üũ Ÿ�̸� ����
			GetWorldTimerManager().ClearTimer(StageTimerHandle);
			GetWorldTimerManager().ClearTimer(GameOverCheckTimerHandle);
			return; // ���ӿ��� ó�� �� �Լ� ����
		}
		// ������ �׾��ٸ� ���������� ���� ���̺� ����
	}
	// --- ���� Ÿ�Ӿƿ� üũ �� ---


	// ���� ���̺� ��ȣ ����
	MyGameState->CurrentWave++;
	MyGameState->OnRep_CurrentWave(); // RepNotify ȣ�� (���� UI ��� ������Ʈ �� Ŭ���̾�Ʈ ����)

	// ��� �÷��̾�� ���� ���� Ƚ�� 2ȸ �ο� (���� ����)
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		if (MyPS)
		{
			// AddChoiceCount �Լ� �̸� Ȯ�� �ʿ� (���� �ڵ忡���� AddChoiceCount�� �Ǿ� �־���)
			MyPS->AddChoiceCount(2); // ���� ���� �Լ� ȣ��
		}
	}

	// --- [�ڵ� ����] ���� ���̺� �б� ���� ---
	const int32 CurrentWave = MyGameState->GetCurrentWave();
	// ���� ���̺갡 1 �̻��̰� 10�� ����̸� ���� ���̺�� ����
	const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);
	// ���� ���̺�� 120��, �Ϲ� ���̺�� 60��
	const float TimeLimitForThisWave = bIsBossWave ? BossStageTimeLimit : StageTimeLimit;

	// GameState�� �̹� ���̺��� ���� �ð� ��� (���� ����)
	MyGameState->WaveEndTime = GetWorld()->GetTimeSeconds() + TimeLimitForThisWave;
	MyGameState->OnRep_WaveEndTime();

	// ���� ���̺�� 1����, �Ϲ� ���̺�� ������(MonstersPerWave)��ŭ ����
	const int32 NumToSpawn = bIsBossWave ? 1 : MonstersPerWave;
	// --- ���� ���̺� �б� �� ---

	// �α� ��� (���� ����)
	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
	RID_LOG(FColor::White, TEXT("Wave %d Info: bIsBoss=%s, TimeLimit=%.1f, NumToSpawn=%d"),
		CurrentWave, bIsBossWave ? TEXT("true") : TEXT("false"), TimeLimitForThisWave, NumToSpawn);
	// -----------------------------------------

	// ���� ������ ���̺� �ε� �� ���� ��� (���� ����)
	if (MonsterWaveDataTable)
	{
		// ���� ���̺�� �Ϲ� ���̺꿡 �ٸ� Row Name ��Ģ�� �ʿ��� �� ���� (��: "Boss1", "Boss2")
		// ���⼭�� �ϴ� "Wave10", "Wave20" ������ �����ϰ� ���
		FString RowName = FString::Printf(TEXT("Wave%d"), CurrentWave);
		FMonsterWaveData* WaveData = MonsterWaveDataTable->FindRow<FMonsterWaveData>(*RowName, TEXT(""));

		if (WaveData && WaveData->MonstersToSpawn.Num() > 0)
		{
			// TODO: ���� ���̺��� ��� WaveData->MonstersToSpawn[0]�� ���� ���� ���� Ŭ�������� Ȯ�� �ʿ�
			TSubclassOf<AMonsterBaseCharacter> MonsterClass = WaveData->MonstersToSpawn[0];

			// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
			// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
			RID_LOG(FColor::Cyan, TEXT("Found %d Spawners. Issuing spawn command..."), MonsterSpawners.Num());
			// -----------------------------------------

			// ��� �����ʿ��� ���� ��� ����
			for (AMonsterSpawner* Spawner : MonsterSpawners)
			{
				if (Spawner)
				{
					// �����ʰ� ���ӿ��� ���°� �ƴ� ���� �����ϵ��� �߰� üũ ����
					// if (!Spawner->IsGameOver())
					// {
					Spawner->BeginSpawning(MonsterClass, NumToSpawn);
					// }
				}
			}

			// TODO: ���� ���� �� GameState � ���� ���� ���� �����ϴ� ���� �ʿ� (Ÿ�Ӿƿ� üũ��)

		}
		else
		{
			// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
			RID_LOG(FColor::Red, TEXT("ERROR: WaveData for '%s' not found or no monsters assigned in DataTable!"), *RowName);
			// -----------------------------------------
			// ���̺� �����Ͱ� ������ ���� ���̺�� �Ѿ�� �ʵ��� Ÿ�̸� ���� �ߴ� (���� ����)
			// return;
		}
	}

	// ���� ���̺� ���� Ÿ�̸� ���� (���� ����)
	GetWorld()->GetTimerManager().SetTimer(StageTimerHandle, this, &ARamdomItemDefenseGameMode::StartNextWave, TimeLimitForThisWave, false);
}

void ARamdomItemDefenseGameMode::CheckGameOver()
{
	// ��ϵ� ��� �����ʸ� ��ȸ�ϸ� �˻��մϴ�.
	for (AMonsterSpawner* Spawner : MonsterSpawners)
	{
		// �����ʰ� ��ȿ�ϰ�, ���� ���ӿ��� ���°� �ƴϸ�, ���� ���� 60������ �ʰ��ߴ��� Ȯ��
		if (Spawner && !Spawner->IsGameOver() && Spawner->GetCurrentMonsterCount() > GameoverMonsterNum)
		{
			// �ش� �����ʸ� ���ӿ��� ���·� ����ϴ�.
			Spawner->SetGameOver();

			APlayerController* Controller = GetControllerForSpawner(Spawner);
			ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(Controller);
			if (PC)
			{
				// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
				RID_LOG(FColor::Red, TEXT("GAME OVER: Monster limit exceeded for %s"), *PC->GetName());
				// -----------------------------------------
				PC->ShowGameOverUI();
			}

			// TODO: �� �����ʿ� ����� �÷��̾�� ���ӿ��� UI�� ���� ����
		}
	}
}

/** ������ -> ��Ʈ�ѷ� ã�� ���� �Լ� */
APlayerController* ARamdomItemDefenseGameMode::GetControllerForSpawner(AMonsterSpawner* Spawner) const
{
	if (!Spawner) return nullptr;
	AMyGameState* MyGameState = GetGameState<AMyGameState>();
	if (!MyGameState) return nullptr;

	// ��� PlayerState�� ��ȸ
	for (APlayerState* PS : MyGameState->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);
		// PlayerState�� MySpawner�� �־��� Spawner�� ��ġ�ϴ��� Ȯ��
		if (MyPS && MyPS->MySpawner == Spawner)
		{
			// ��ġ�ϸ� �ش� PlayerState�� Controller ��ȯ
			return MyPS->GetPlayerController();
		}
	}
	return nullptr; // ã�� ����
}