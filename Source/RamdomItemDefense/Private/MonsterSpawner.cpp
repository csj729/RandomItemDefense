
// csj729/randomitemdefense/RandomItemDefense-78a128504f0127dc02646504d4a1e1c677a0e811/Source/RamdomItemDefense/Private/MonsterSpawner.cpp
#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "RamdomItemDefense.h" // RID_LOG 매크로용
#include "MyGameState.h"
#include "Materials/MaterialInterface.h"
#include "AbilitySystemComponent.h" 
#include "GameplayTagContainer.h" 


AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

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
	// 서버에서만 스폰
	if (!HasAuthority()) return;

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
				SpawnedMonster->SetSpawner(this);
				CurrentMonsterCount++;
				OnRep_CurrentMonsterCount(); // 서버 UI 즉시 업데이트

				// 1. GameState에서 현재 웨이브 가져오기
				AMyGameState* MyGameState = World->GetGameState<AMyGameState>();
				if (MyGameState)
				{
					int32 CurrentWave = MyGameState->GetCurrentWave(); // (1부터 시작)

					SpawnedMonster->SetSpawnWaveIndex(CurrentWave);

					// --- 스탯 적용 로직 ---
					UAbilitySystemComponent* MonsterASC = SpawnedMonster->GetAbilitySystemComponent();
					if (MonsterASC)
					{
						MonsterASC->InitAbilityActorInfo(SpawnedMonster, SpawnedMonster);

						if (MonsterStatInitEffect)
						{
							FGameplayEffectContextHandle ContextHandle = MonsterASC->MakeEffectContext();
							ContextHandle.AddSourceObject(this);
							FGameplayEffectSpecHandle SpecHandle = MonsterASC->MakeOutgoingSpec(MonsterStatInitEffect, 1.0f, ContextHandle);

							if (SpecHandle.IsValid())
							{
								float BaseHP = MONSTER_BASE_HP;
								float FinalHP = BaseHP + FMath::Max(0.f, (float)(CurrentWave - 1) * 50.f);
								SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusHP")), FinalHP);

								int FinalArmor = 0;
								const int32 BossStage = CurrentWave / 10;
								const float BaseArmor = BossStage * 20.0f;
								const int32 WaveNumInBlock = (CurrentWave % 10);
								FinalArmor = BaseArmor + WaveNumInBlock;
								SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusArmor")), FinalArmor);

								MonsterASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
							}
						}
					}

					// --- 머티리얼 적용 로직 ---
					const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);
					if (!bIsBossWave)
					{
						const TArray<TObjectPtr<UMaterialInterface>>& WaveMaterials = SpawnedMonster->GetWaveMaterials();
						if (WaveMaterials.Num() == 9)
						{
							int32 MaterialIndex = (CurrentWave % 10) - 1;
							if (MaterialIndex >= 0 && MaterialIndex < WaveMaterials.Num())
							{
								UMaterialInterface* MaterialToApply = WaveMaterials[MaterialIndex];
								if (MaterialToApply)
								{
									SpawnedMonster->SetWaveMaterial(MaterialToApply);
								}
							}
						}
					}
				}
			}

			SpawnCounter++;

			if (bEnableDebug)
			{
				DrawDebugSphere(GetWorld(), GetActorLocation(), 100.0f, 12, FColor::Green, false, 2.0f);
			}
		}
	}
	else
	{
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
}

void AMonsterSpawner::SpawnCounterAttackMonster(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 MonsterWaveIndex)
{
	if (!HasAuthority() || !MonsterClass) return;
	if (bIsGameOver) return; // 게임오버 상태면 무시

	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		// 현재 스포너 위치에서 즉시 스폰
		AMonsterBaseCharacter* SpawnedMonster = World->SpawnActor<AMonsterBaseCharacter>(MonsterClass, GetActorLocation(), GetActorRotation(), SpawnParams);

		if (SpawnedMonster)
		{
			SpawnedMonster->SetSpawner(this);
			SpawnedMonster->SetIsCounterAttackMonster(true); // [중요] 반격 몬스터 플래그
			CurrentMonsterCount++;
			OnRep_CurrentMonsterCount();

			// [ ★★★ 핵심 수정 ★★★ ]
			// GameState->GetCurrentWave() 대신, 인자로 받은 'MonsterWaveIndex'를 사용합니다.
			int32 TargetWaveIndex = MonsterWaveIndex;

			// (안전 장치: 혹시 0 이하가 들어오면 1로 보정)
			if (TargetWaveIndex <= 0) TargetWaveIndex = 1;

			SpawnedMonster->SetSpawnWaveIndex(TargetWaveIndex);

			// 스탯 적용 로직에도 TargetWaveIndex를 사용
			UAbilitySystemComponent* MonsterASC = SpawnedMonster->GetAbilitySystemComponent();
			if (MonsterASC)
			{
				MonsterASC->InitAbilityActorInfo(SpawnedMonster, SpawnedMonster);
				if (MonsterStatInitEffect)
				{
					FGameplayEffectContextHandle ContextHandle = MonsterASC->MakeEffectContext();
					ContextHandle.AddSourceObject(this);
					FGameplayEffectSpecHandle SpecHandle = MonsterASC->MakeOutgoingSpec(MonsterStatInitEffect, 1.0f, ContextHandle);

					if (SpecHandle.IsValid())
					{
						// [수정] TargetWaveIndex 기준 스탯 계산
						float BaseHP = MONSTER_BASE_HP;
						float FinalHP = BaseHP + FMath::Max(0.f, (float)(TargetWaveIndex - 1) * 50.f);
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusHP")), FinalHP);

						int FinalArmor = 0;
						const int32 BossStage = TargetWaveIndex / 10;
						const float BaseArmor = BossStage * 20.0f;
						const int32 WaveNumInBlock = (TargetWaveIndex % 10);
						FinalArmor = BaseArmor + WaveNumInBlock;
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusArmor")), FinalArmor);

						MonsterASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					}
				}
			}

			// 로그 확인
			RID_LOG(FColor::Red, TEXT("Spawner: Counter-Attack Monster Spawned! Class: %s (Wave: %d)"),
				*GetNameSafe(MonsterClass), TargetWaveIndex);
		}
	}
}