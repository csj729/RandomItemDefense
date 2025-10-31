// csj729/randomitemdefense/RandomItemDefense-78a128504f0127dc02646504d4a1e1c677a0e811/Source/RamdomItemDefense/Private/MonsterSpawner.cpp
#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "RamdomItemDefense.h" // RID_LOG ��ũ�ο�
#include "MyGameState.h"
#include "Materials/MaterialInterface.h"
#include "AbilitySystemComponent.h" // ASC ����� ���� �߰�
#include "GameplayTagContainer.h" // FGameplayTag ����� ���� �߰�


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

	if (bEnableDebug)
	{
		// FString MonsterName = GetNameSafe(MonsterClassToSpawn);
		// RID_LOG(FColor::Cyan, TEXT("Spawner '%s' received command: Spawn %d of '%s'."), *GetName(), Count, *MonsterName); // [�α� ����]
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
	// ���������� ����
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
				OnRep_CurrentMonsterCount(); // ���� UI ��� ������Ʈ

				// 1. GameState���� ���� ���̺� ��������
				AMyGameState* MyGameState = World->GetGameState<AMyGameState>();
				if (MyGameState)
				{
					int32 CurrentWave = MyGameState->GetCurrentWave(); // (1���� ����)

					// --- [ �ڡڡ� ���̺� �����ϸ� ���� ���� ���� (������) �ڡڡ� ] ---
					UAbilitySystemComponent* MonsterASC = SpawnedMonster->GetAbilitySystemComponent();
					if (MonsterASC)
					{
						// 2. (�߿�) ������ ������ ASC�� ��� �ʱ�ȭ�մϴ�.
						// (BeginPlay���� ���� ����ǹǷ�, ���⼭ �������� ����� �մϴ�)
						MonsterASC->InitAbilityActorInfo(SpawnedMonster, SpawnedMonster);

						if (MonsterStatInitEffect) // (h���Ͽ� �߰��� ����)
						{
							// 3. ����(Spec) ����
							FGameplayEffectContextHandle ContextHandle = MonsterASC->MakeEffectContext();
							ContextHandle.AddSourceObject(this);
							FGameplayEffectSpecHandle SpecHandle = MonsterASC->MakeOutgoingSpec(MonsterStatInitEffect, 1.0f, ContextHandle);

							if (SpecHandle.IsValid())
							{
								// 4. ���̺� ���ʽ� HP ��� (Wave 1 = 0, Wave 2 = 50, ...)
								// (�䱸����: ������������ 50�� ���)
								// (CurrentWave 1 ���� 0)
								float BonusHP = FMath::Max(0.f, (float)(CurrentWave - 1) * 50.f);

								// 5. SetByCaller�� ���ʽ� HP �� ����
								// (����: �±� �̸��� "Data.Wave.BonusHP"�� ��Ȯ�ؾ� ��)
								SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Wave.BonusHP")), BonusHP);

								// 6. ���� ����
								MonsterASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

								//RID_LOG(FColor::Green, TEXT("Applied GE_MonsterStatInit to %s. Wave: %d, BonusHP: %.1f"), *SpawnedMonster->GetName(), CurrentWave, BonusHP);
							}
						}
						else
						{
							RID_LOG(FColor::Red, TEXT("Spawner ERROR: MonsterStatInitEffect is not set on the Spawner BP!"));
						}
					}
					// --- [ ���� ���� ���� �� ] ---

					// [�߰�] ���� ���̺����� Ȯ�� (GameMode ������ ����)
					const bool bIsBossWave = (CurrentWave > 0 && CurrentWave % 10 == 0);

					// [����] ���� ���̺갡 �ƴ� ��(!bIsBossWave)�� ��Ƽ���� ���� ���� ����
					if (!bIsBossWave)
					{
						// 2. ������ ��Ƽ���� ��� ��������
						const TArray<TObjectPtr<UMaterialInterface>>& WaveMaterials = SpawnedMonster->GetWaveMaterials();

						// 3. ��Ƽ���� �迭�� 9��(�ε��� 0~8)���� Ȯ�� (�ſ� �߿�)
						if (WaveMaterials.Num() == 9)
						{
							// 4. ���̺��� '���� �ڸ�' ���ڸ� �ε����� ����մϴ�. (0~8)
							// Wave 1 -> (1 % 10) - 1 = 0 (�ε��� 0)
							// Wave 9 -> (9 % 10) - 1 = 8 (�ε��� 8)
							// Wave 11 -> (11 % 10) - 1 = 0 (�ε��� 0)
							int32 MaterialIndex = (CurrentWave % 10) - 1;

							// (Ȥ�� �� ���� �ε��� ���� - ������ �߻� �� ��)
							if (MaterialIndex >= 0 && MaterialIndex < WaveMaterials.Num())
							{
								UMaterialInterface* MaterialToApply = WaveMaterials[MaterialIndex];

								// 5. ���Ϳ� ��Ƽ���� ���� (���� ���� �Լ� ȣ��)
								if (MaterialToApply)
								{
									SpawnedMonster->SetWaveMaterial(MaterialToApply);
								}
								else
								{
									// (����� �α�) �ش� �ε����� ��Ƽ������ �������
									// RID_LOG(FColor::Yellow, TEXT("Spawner: WaveMaterials[%d] is NULL for Wave %d."), MaterialIndex, CurrentWave); // [�α� ����]
								}
							}
						}
						else
						{
							// (����� �α�) ���� BP�� ��Ƽ������ 9�� �������� ����
							// RID_LOG(FColor::Red, TEXT("Spawner ERROR: Monster BP '%s' must have exactly 9 elements in WaveMaterials (found %d)."), // [�α� ����]
								// *GetNameSafe(MonsterClassToSpawn), WaveMaterials.Num()); // [�α� ����]
						}
					}
					// (else: ���� ���̺��� ���, �ƹ��͵� ���� �ʰ� ������ �⺻ ��Ƽ������ ����մϴ�)
				}
				// --- [�ڵ� ���� ��] ---
			}

			SpawnCounter++;

			if (bEnableDebug)
			{
				// RID_LOG(FColor::Green, TEXT("Spawner '%s' spawned monster #%d/%d. (Live: %d)"), *GetName(), SpawnCounter, TotalToSpawn, CurrentMonsterCount); // [�α� ����]
				DrawDebugSphere(GetWorld(), GetActorLocation(), 100.0f, 12, FColor::Green, false, 2.0f);
			}
		}
	}
	else
	{
		if (bEnableDebug)
		{
			// RID_LOG(FColor::White, TEXT("Spawner '%s' finished spawning wave."), *GetName()); // [�α� ����]
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
		// RID_LOG(FColor::Magenta, TEXT("Spawner '%s' is now in GAME OVER state."), *GetName()); // [�α� ����]
	}
}