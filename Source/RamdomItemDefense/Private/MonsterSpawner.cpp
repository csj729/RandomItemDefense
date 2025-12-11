// Source/RamdomItemDefense/Private/MonsterSpawner.cpp

#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "MonsterAIController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "RamdomItemDefense.h"
#include "MyGameState.h"
#include "Materials/MaterialInterface.h"
#include "AbilitySystemComponent.h" 
#include "Kismet/GameplayStatics.h"
#include "GameplayTagContainer.h" 


AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 게임 로직에 필수적인 관리자 액터이므로 Culling되면 안 됩니다.
	bAlwaysRelevant = true;

	// 스포너 상태가 자주 바뀌지 않는다면 기본값도 괜찮지만, 디버깅을 위해 명시합니다.
	NetUpdateFrequency = 100.0f;

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

			// 1. 지연 스폰 시작
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
				// GameState에서 현재 웨이브 정보를 가져와 몬스터에게 설정
				int32 CurrentWave = 1;
				if (AMyGameState* MyGameState = World->GetGameState<AMyGameState>())
				{
					CurrentWave = MyGameState->GetCurrentWave();
				}

				// 2. 지연 스폰 마무리 (BeginPlay 실행됨)
				UGameplayStatics::FinishSpawningActor(SpawnedMonster, SpawnTransform);

				// 3. [리팩토링] 공통 초기화 로직 호출
				InitSpawnedMonster(SpawnedMonster, CurrentWave);
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

void AMonsterSpawner::SpawnCounterAttackMonster(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 MonsterWaveIndex)
{
	if (!HasAuthority() || !MonsterClass) return;
	if (bIsGameOver) return;

	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AMonsterBaseCharacter* SpawnedMonster = World->SpawnActor<AMonsterBaseCharacter>(
			MonsterClass, GetActorLocation(), GetActorRotation(), SpawnParams);

		if (SpawnedMonster)
		{
			// 반격 몬스터 설정
			SpawnedMonster->SetIsCounterAttackMonster(true);

			// 0 이하의 웨이브가 들어올 경우 1로 보정
			int32 TargetWaveIndex = (MonsterWaveIndex <= 0) ? 1 : MonsterWaveIndex;

			// [리팩토링] 공통 초기화 로직 호출 (SetSpawner, 스탯 적용 등 포함)
			InitSpawnedMonster(SpawnedMonster, TargetWaveIndex);
		}
		else
		{
			RID_LOG(FColor::Red, TEXT("Counter Monster Spawn FAILED! (SpawnActor returned null)"));
		}
	}
}

void AMonsterSpawner::InitSpawnedMonster(AMonsterBaseCharacter* SpawnedMonster, int32 WaveIndex)
{
	if (!SpawnedMonster) return;

	// 1. 스포너 및 웨이브 정보 설정
	// (SetSpawner 내부에서 AIController에게 PatrolPath도 같이 전달합니다)
	SpawnedMonster->SetSpawner(this);
	SpawnedMonster->SetSpawnWaveIndex(WaveIndex);

	// 2. 관리 카운트 증가 및 UI 갱신
	CurrentMonsterCount++;
	OnRep_CurrentMonsterCount();

	// 3. 스탯 및 외형 적용
	ApplyMonsterStatsByWave(SpawnedMonster, WaveIndex);
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

void AMonsterSpawner::ApplyMonsterStatsByWave(AMonsterBaseCharacter* Monster, int32 WaveIndex)
{
	if (!Monster) return;

	// -------------------------------------------------------
	// 1. GAS 스탯 (체력, 방어력) 적용
	// -------------------------------------------------------
	if (MonsterStatInitEffect)
	{
		UAbilitySystemComponent* MonsterASC = Monster->GetAbilitySystemComponent();
		if (MonsterASC)
		{
			MonsterASC->InitAbilityActorInfo(Monster, Monster);

			FGameplayEffectContextHandle ContextHandle = MonsterASC->MakeEffectContext();
			ContextHandle.AddSourceObject(this);
			FGameplayEffectSpecHandle SpecHandle = MonsterASC->MakeOutgoingSpec(MonsterStatInitEffect, 1.0f, ContextHandle);

			if (SpecHandle.IsValid())
			{
				const int32 BossStage = WaveIndex / 10;
				float FinalHP = 0.f;

				// [체력 계산] 보스 웨이브인지 확인 (10, 20, 30...)
				if (WaveIndex > 0 && WaveIndex % 10 == 0)
				{
					float BaseBossHP = 50000.0f;
					float AddedHP = 100000.0f * (float)BossStage * (float)(BossStage - 1);

					FinalHP = BaseBossHP + AddedHP;
				}
				else
				{
					// 일반 몬스터 (기존 로직 유지)
					float HP_Multiplier = 1.0f + (BossStage * 0.5f);
					FinalHP = Monster->MaxHealth + (FMath::Max(0.f, (float)(WaveIndex - 1) * 50.f) * HP_Multiplier);
				}

				SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusHP")), FinalHP);

				// [방어력]
				const float BaseArmor = BossStage * 30.0f;
				const int32 WaveNumInBlock = (WaveIndex % 10);
				int FinalArmor = BaseArmor + (WaveNumInBlock * 2);

				SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusArmor")), FinalArmor);

				MonsterASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// -------------------------------------------------------
	// 2. 머티리얼 (외형) 적용
	// -------------------------------------------------------
	// 보스 웨이브(10, 20...)가 아닐 때만 웨이브 색상 적용
	const bool bIsBossWave = (WaveIndex > 0 && WaveIndex % 10 == 0);

	if (!bIsBossWave)
	{
		const TArray<TObjectPtr<UMaterialInterface>>& WaveMaterials = Monster->GetWaveMaterials();

		// 웨이브 1~9는 인덱스 0~8에 매핑
		int32 MaterialIndex = (WaveIndex % 10) - 1;

		if (WaveMaterials.Num() > 0 && MaterialIndex >= 0 && MaterialIndex < WaveMaterials.Num())
		{
			UMaterialInterface* MaterialToApply = WaveMaterials[MaterialIndex];
			if (MaterialToApply)
			{
				Monster->SetWaveMaterial(MaterialToApply);
			}
		}
	}
}