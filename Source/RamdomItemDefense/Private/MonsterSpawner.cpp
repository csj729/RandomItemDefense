
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
#include "Kismet/GameplayStatics.h"
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
			FTransform SpawnTransform(GetActorRotation(), GetActorLocation());
			// 1. [핵심 수정] SpawnActorDeferred를 사용하여 '지연 스폰' 시작
			// 이 시점에는 아직 BeginPlay와 AIController 빙의(Possess)가 실행되지 않습니다.
			AMonsterBaseCharacter* SpawnedMonster = World->SpawnActorDeferred<AMonsterBaseCharacter>(
				MonsterClassToSpawn,
				SpawnTransform,
				this,           // Owner
				GetInstigator(), // Instigator
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
			);

			if (SpawnedMonster)
			{
				// 2. [데이터 주입] AI가 켜지기 전에 필수 데이터를 먼저 넣어줍니다.
				// 이렇게 하면 AIController가 BeginPlay에서 블랙보드 값을 읽을 때 null이 아닙니다.
				SpawnedMonster->SetSpawner(this);

				// GameState에서 현재 웨이브 정보를 가져와 몬스터에게 설정
				if (AMyGameState* MyGameState = World->GetGameState<AMyGameState>())
				{
					int32 CurrentWave = MyGameState->GetCurrentWave();
					SpawnedMonster->SetSpawnWaveIndex(CurrentWave);
				}

				// 3. [스폰 마무리] 이제 스폰을 완료합니다.
				// 이때 비로소 BeginPlay -> OnPossess -> AI Behavior Tree가 실행됩니다.
				UGameplayStatics::FinishSpawningActor(SpawnedMonster, SpawnTransform);

				// --- [기존 로직 유지] ---
				CurrentMonsterCount++;
				OnRep_CurrentMonsterCount(); // 서버 UI 즉시 업데이트

				// ASC(Ability System Component) 초기화 및 스탯 적용
				// (FinishSpawningActor 이후에 해야 안전합니다)
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
							// GameState를 다시 가져오거나 위에서 저장한 CurrentWave 사용
							// (여기서는 안전하게 다시 가져오거나 위 변수를 활용)
							int32 CurrentWave = SpawnedMonster->GetSpawnWaveIndex(); // 방금 설정했으므로 가져올 수 있음

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
				// (FinishSpawningActor 이후에 실행되어야 머티리얼이 정상 적용됨)
				int32 CurrentWave = SpawnedMonster->GetSpawnWaveIndex();
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

			// 스폰 카운트 증가
			SpawnCounter++;
		}
	}
	else
	{
		// 스폰 할당량을 다 채웠으면 타이머 정지
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