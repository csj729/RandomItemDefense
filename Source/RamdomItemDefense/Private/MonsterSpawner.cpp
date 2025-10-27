// csj729/randomitemdefense/RandomItemDefense-78a128504f0127dc02646504d4a1e1c677a0e811/Source/RamdomItemDefense/Private/MonsterSpawner.cpp
#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "RamdomItemDefense.h" // RID_LOG 매크로용

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
	// UI에 몬스터 수가 변경되었음을 알립니다.
	OnMonsterCountChangedDelegate.Broadcast(CurrentMonsterCount);
}

void AMonsterSpawner::BeginSpawning(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 Count)
{
	// 만약 이 스테이지가 게임오버 상태라면, 아무것도 하지 않고 즉시 함수를 종료합니다.
	if (bIsGameOver)
	{
		return;
	}

	MonsterClassToSpawn = MonsterClass;
	TotalToSpawn = Count;
	SpawnCounter = 0;

	// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
	if (bEnableDebug)
	{
		FString MonsterName = GetNameSafe(MonsterClassToSpawn);
		// [수정] FString 변수를 만들지 않고 매크로에 직접 전달합니다.
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
				// 스폰된 몬스터에게 "너를 스폰한 스포너는 나야" 라고 알려줍니다.
				SpawnedMonster->SetSpawner(this);
				CurrentMonsterCount++; // 스폰 성공 시 카운트 증가
				if (HasAuthority()) OnRep_CurrentMonsterCount();
			}

			SpawnCounter++;

			if (bEnableDebug)
			{
				// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
				// [수정] FString 변수를 만들지 않고 매크로에 직접 전달합니다.
				RID_LOG(FColor::Green, TEXT("Spawner '%s' spawned monster #%d/%d. (Live: %d)"), *GetName(), SpawnCounter, TotalToSpawn, CurrentMonsterCount);
				// -----------------------------------------
				DrawDebugSphere(GetWorld(), GetActorLocation(), 100.0f, 12, FColor::Green, false, 2.0f);
			}
		}
	}
	else
	{
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		if (bEnableDebug)
		{
			// [수정] FString 변수를 만들지 않고 매크로에 직접 전달합니다.
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

	// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
	if (bEnableDebug)
	{
		// [수정] FString 변수를 만들지 않고 매크로에 직접 전달합니다.
		RID_LOG(FColor::Magenta, TEXT("Spawner '%s' is now in GAME OVER state."), *GetName());
	}
	// -----------------------------------------
}