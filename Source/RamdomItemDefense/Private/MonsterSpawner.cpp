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
#include "AbilitySystemComponent.h" // ASC 사용을 위해 추가
#include "GameplayTagContainer.h" // FGameplayTag 사용을 위해 추가


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

	if (bEnableDebug)
	{
		// FString MonsterName = GetNameSafe(MonsterClassToSpawn);
		// RID_LOG(FColor::Cyan, TEXT("Spawner '%s' received command: Spawn %d of '%s'."), *GetName(), Count, *MonsterName); // [로그 제거]
	}

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
					// --- [ ★★★ 웨이브 스케일링 스탯 적용 로직 (수정됨) ★★★ ] ---
					UAbilitySystemComponent* MonsterASC = SpawnedMonster->GetAbilitySystemComponent();
					if (MonsterASC)
					{
						// 2. (중요) 스폰된 액터의 ASC를 즉시 초기화합니다.
						// (BeginPlay보다 먼저 실행되므로, 여기서 수동으로 해줘야 합니다)
						MonsterASC->InitAbilityActorInfo(SpawnedMonster, SpawnedMonster);

						if (MonsterStatInitEffect) // (h파일에 추가한 변수)
						{
							// 3. 스펙(Spec) 생성
							FGameplayEffectContextHandle ContextHandle = MonsterASC->MakeEffectContext();
							ContextHandle.AddSourceObject(this);
							FGameplayEffectSpecHandle SpecHandle = MonsterASC->MakeOutgoingSpec(MonsterStatInitEffect, 1.0f, ContextHandle);

							if (SpecHandle.IsValid())
							{
								float BaseHP = MONSTER_BASE_HP;
								// 4. 웨이브 보너스 HP 계산 (Wave 1 = 0, Wave 2 = 50, ...)
								// (요구사항: 스테이지마다 50씩 상승)
								// (CurrentWave 1 기준 0)
								float FinalHP = BaseHP + FMath::Max(0.f, (float)(CurrentWave - 1) * 50.f);

								// 5. SetByCaller로 보너스 HP 값 주입
								// (주의: 태그 이름이 "Data.Wave.BonusHP"로 정확해야 함)
								SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusHP")), FinalHP);
								// --- [ ★★★ 방어력 계산 로직 추가 ★★★ ] ---
								int FinalArmor = 0;

								const int32 BossStage = CurrentWave / 10; // (Wave 1-9 -> 0), (Wave 11-19 -> 1)
								const float BaseArmor = BossStage * 20.0f;

								const int32 WaveNumInBlock = (CurrentWave % 10);
								FinalArmor = BaseArmor + WaveNumInBlock;

								// 6. SetByCaller로 보너스 Armor 값 주입
								SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusArmor")), FinalArmor);
								//RID_LOG(FColor::White, TEXT("SpawnMonster: Wave %d -> HP: %.1f, Armor: %.1f"), CurrentWave, BonusHP, FinalArmor);
								// --- [ ★★★ 방어력 계산 로직 끝 ★★★ ] ---


								// 7. 스펙 적용
								MonsterASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

								//RID_LOG(FColor::Green, TEXT("Applied GE_MonsterStatInit to %s. Wave: %d, BonusHP: %.1f"), *SpawnedMonster->GetName(), CurrentWave, BonusHP);
							}
						}
						else
						{
							RID_LOG(FColor::Red, TEXT("Spawner ERROR: MonsterStatInitEffect is not set on the Spawner BP!"));
						}
					}
					// --- [ 스탯 적용 로직 끝 ] ---

					// [추가] 보스 웨이브인지 확인 (GameMode 로직과 동일)
					const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);

					// [수정] 보스 웨이브가 아닐 때(!bIsBossWave)만 머티리얼 변경 로직 실행
					if (!bIsBossWave)
					{
						// 2. 몬스터의 머티리얼 목록 가져오기
						const TArray<TObjectPtr<UMaterialInterface>>& WaveMaterials = SpawnedMonster->GetWaveMaterials();

						// 3. 머티리얼 배열이 9개(인덱스 0~8)인지 확인 (매우 중요)
						if (WaveMaterials.Num() == 9)
						{
							// 4. 웨이브의 '일의 자리' 숫자를 인덱스로 사용합니다. (0~8)
							// Wave 1 -> (1 % 10) - 1 = 0 (인덱스 0)
							// Wave 9 -> (9 % 10) - 1 = 8 (인덱스 8)
							// Wave 11 -> (11 % 10) - 1 = 0 (인덱스 0)
							int32 MaterialIndex = (CurrentWave % 10) - 1;

							// (혹시 모를 음수 인덱스 방지 - 로직상 발생 안 함)
							if (MaterialIndex >= 0 && MaterialIndex < WaveMaterials.Num())
							{
								UMaterialInterface* MaterialToApply = WaveMaterials[MaterialIndex];

								// 5. 몬스터에 머티리얼 설정 (서버 전용 함수 호출)
								if (MaterialToApply)
								{
									SpawnedMonster->SetWaveMaterial(MaterialToApply);
								}
								else
								{
									// (디버그 로그) 해당 인덱스에 머티리얼이 비어있음
									// RID_LOG(FColor::Yellow, TEXT("Spawner: WaveMaterials[%d] is NULL for Wave %d."), MaterialIndex, CurrentWave); // [로그 제거]
								}
							}
						}
						else
						{
							// (디버그 로그) 몬스터 BP에 머티리얼이 9개 설정되지 않음
							// RID_LOG(FColor::Red, TEXT("Spawner ERROR: Monster BP '%s' must have exactly 9 elements in WaveMaterials (found %d)."), // [로그 제거]
								// *GetNameSafe(MonsterClassToSpawn), WaveMaterials.Num()); // [로그 제거]
						}
					}
					// (else: 보스 웨이브인 경우, 아무것도 하지 않고 몬스터의 기본 머티리얼을 사용합니다)
				}
				// --- [코드 수정 끝] ---
			}

			SpawnCounter++;

			if (bEnableDebug)
			{
				// RID_LOG(FColor::Green, TEXT("Spawner '%s' spawned monster #%d/%d. (Live: %d)"), *GetName(), SpawnCounter, TotalToSpawn, CurrentMonsterCount); // [로그 제거]
				DrawDebugSphere(GetWorld(), GetActorLocation(), 100.0f, 12, FColor::Green, false, 2.0f);
			}
		}
	}
	else
	{
		if (bEnableDebug)
		{
			// RID_LOG(FColor::White, TEXT("Spawner '%s' finished spawning wave."), *GetName()); // [로그 제거]
		}
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

	if (bEnableDebug)
	{
		// RID_LOG(FColor::Magenta, TEXT("Spawner '%s' is now in GAME OVER state."), *GetName()); // [로그 제거]
	}
}