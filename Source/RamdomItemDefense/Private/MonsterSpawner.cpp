// csj729/randomitemdefense/RandomItemDefense-78a128504f0127dc02646504d4a1e1c677a0e811/Source/RamdomItemDefense/Private/MonsterSpawner.cpp
#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "RamdomItemDefense.h" // RID_LOG ��ũ�ο�

AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	TotalToSpawn = 0;
	SpawnCounter = 0;
	bEnableDebug = true;
	CurrentMonsterCount = 0;
	bIsGameOver = false;
}

void AMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void AMonsterSpawner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMonsterSpawner, CurrentMonsterCount);
}

void AMonsterSpawner::OnRep_CurrentMonsterCount()
{
	// UI�� ���� ���� ����Ǿ����� �˸��ϴ�.
	OnMonsterCountChangedDelegate.Broadcast(CurrentMonsterCount);
}

void AMonsterSpawner::BeginSpawning(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 Count)
{
	// ���� �� ���������� ���ӿ��� ���¶��, �ƹ��͵� ���� �ʰ� ��� �Լ��� �����մϴ�.
	if (bIsGameOver)
	{
		return;
	}

	MonsterClassToSpawn = MonsterClass;
	TotalToSpawn = Count;
	SpawnCounter = 0;

	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	if (bEnableDebug)
	{
		FString MonsterName = GetNameSafe(MonsterClassToSpawn);
		// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
		RID_LOG(FColor::Cyan, TEXT("Spawner '%s' received command: Spawn %d of '%s'."), *GetName(), Count, *MonsterName);
	}
	// -----------------------------------------

	if (MonsterClassToSpawn && TotalToSpawn > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AMonsterSpawner::SpawnMonster,
			1.0f,
			true,
			0.0f
		);
	}
}

void AMonsterSpawner::SpawnMonster()
{
	if (SpawnCounter < TotalToSpawn && MonsterClassToSpawn)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();

			AMonsterBaseCharacter* SpawnedMonster = World->SpawnActor<AMonsterBaseCharacter>(MonsterClassToSpawn, GetActorLocation(), GetActorRotation(), SpawnParams);

			if (SpawnedMonster)
			{
				// ������ ���Ϳ��� "�ʸ� ������ �����ʴ� ����" ��� �˷��ݴϴ�.
				SpawnedMonster->SetSpawner(this);
				CurrentMonsterCount++; // ���� ���� �� ī��Ʈ ����
				if (HasAuthority()) OnRep_CurrentMonsterCount();
			}

			SpawnCounter++;

			if (bEnableDebug)
			{
				// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
				// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
				RID_LOG(FColor::Green, TEXT("Spawner '%s' spawned monster #%d/%d. (Live: %d)"), *GetName(), SpawnCounter, TotalToSpawn, CurrentMonsterCount);
				// -----------------------------------------
				DrawDebugSphere(GetWorld(), GetActorLocation(), 100.0f, 12, FColor::Green, false, 2.0f);
			}
		}
	}
	else
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		if (bEnableDebug)
		{
			// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
			RID_LOG(FColor::White, TEXT("Spawner '%s' finished spawning wave."), *GetName());
		}
		// -----------------------------------------
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

void AMonsterSpawner::OnMonsterKilled()
{
	if (CurrentMonsterCount > 0)
	{
		CurrentMonsterCount--;
		if (HasAuthority()) OnRep_CurrentMonsterCount();
	}
}

void AMonsterSpawner::SetGameOver()
{
	bIsGameOver = true;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	if (bEnableDebug)
	{
		// [����] FString ������ ������ �ʰ� ��ũ�ο� ���� �����մϴ�.
		RID_LOG(FColor::Magenta, TEXT("Spawner '%s' is now in GAME OVER state."), *GetName());
	}
	// -----------------------------------------
}