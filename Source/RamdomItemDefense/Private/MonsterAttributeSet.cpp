// Source/RamdomItemDefense/Private/MonsterAttributeSet.cpp (수정)

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h" 
#include "RamdomItemDefenseCharacter.h"
#include "RamdomItemDefenseGameMode.h"
#include "MyPlayerState.h"
#include "RamdomItemDefense.h" // <--- UE_LOG(LogRamdomItemDefense, ...)를 위해 필요
#include "MyGameState.h"       
#include "Engine/World.h"      
#include "Kismet/KismetMathLibrary.h" // FMath::Pow() 함수 사용
#include "AbilitySystemBlueprintLibrary.h"

void UMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}


float UMonsterAttributeSet::CalculateReducedDamage(float IncomingDamage) const
{
	const float CurrentArmor = FMath::Max(0.f, GetArmor());
	if (CurrentArmor <= 0.f)
	{
		return IncomingDamage;
	}

	float DamageReductionPercent = 0.05f * FMath::Pow(CurrentArmor, 0.4515f);
	DamageReductionPercent = FMath::Min(DamageReductionPercent, 0.9f);

	const float ReducedDamage = IncomingDamage * (1.0f - DamageReductionPercent);
	return ReducedDamage;
}


void UMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float Magnitude = Data.EvaluatedData.Magnitude;
		const float NewHealth = GetHealth();

		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		if (Monster && !Monster->IsDying())
		{
			if (NewHealth <= 0.f)
			{
				// [1] 킬러 액터 확인
				AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
				AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();
				AActor* KillerActor = InstigatorActor ? InstigatorActor : EffectCauserActor;

				// [2] 몬스터 사망 처리 (KillerActor 전달)
				Monster->Die(KillerActor);

				// [3] ★★★ 킬러의 PlayerState 찾기 (드론 등 소환수 고려) ★★★
				AMyPlayerState* KillerPS = nullptr;
				if (KillerActor)
				{
					// 3-1. 킬러가 폰이라면 바로 PS 가져오기
					if (APawn* KillerPawn = Cast<APawn>(KillerActor))
					{
						KillerPS = KillerPawn->GetPlayerState<AMyPlayerState>();
					}

					// 3-2. PS가 없다면(예: 드론), 주인을 찾아서 PS 가져오기
					if (!KillerPS)
					{
						if (APawn* OwnerPawn = Cast<APawn>(KillerActor->GetOwner()))
						{
							KillerPS = OwnerPawn->GetPlayerState<AMyPlayerState>();
						}
					}
				}

				// [4] 보상 및 반격 로직 (KillerPS가 존재하면 실행)
				if (KillerPS)
				{
					// --- [골드 지급] ---
					int32 CurrentWave = Monster->GetSpawnWaveIndex();
					if (CurrentWave <= 0) CurrentWave = 1;

					int32 BaseGold = 0;
					int32 BonusGold = 0;
					int32 FinalGold = 0;

					if (Monster->IsBoss())
					{
						const int32 BossStage = CurrentWave / 10;
						const int32 ItemChoiceReward = BossStage * 3;
						KillerPS->AddCommonItemChoice(ItemChoiceReward);
						BaseGold = 3000;
						BonusGold = (BossStage - 1) * 4000;
					}
					else
					{
						BaseGold = 10;
						BonusGold = (CurrentWave - 1) * 5;
					}

					FinalGold = BaseGold + BonusGold;

					// (보너스 태그 확인: 킬러 액터의 ASC를 확인해야 함)
					UAbilitySystemComponent* KillerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(KillerActor);
					if (KillerASC && KillerASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("ButtonAction.Reward.Gold"))))
					{
						FinalGold *= 1.5f;
					}

					KillerPS->AddGold(FMath::Max(BaseGold, FinalGold));


					// --- [PVP 반격 몬스터 전송] ---
					// 보스가 아니고, 반격으로 태어난 몬스터가 아닐 때만
					if (!Monster->IsBoss() && !Monster->IsCounterAttackMonster())
					{
						if (UWorld* World = Monster->GetWorld())
						{
							ARamdomItemDefenseGameMode* GM = Cast<ARamdomItemDefenseGameMode>(World->GetAuthGameMode());
							if (GM)
							{
								// [수정] 찾은 KillerPS와 몬스터의 WaveIndex를 전달
								int32 MyWaveIndex = Monster->GetSpawnWaveIndex();
								GM->SendCounterAttackMonster(KillerPS, Monster->GetClass(), MyWaveIndex);
							}
						}
					}
				}

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);
				Monster->PlayHitEffect(EffectTags);
			}

			else if (Magnitude < 0.f)
			{
				// [2] --- 몬스터 피격 (데미지) 로직 ---
				const float OriginalDamage = Magnitude;
				const float HealthBeforeDamage = NewHealth - OriginalDamage;
				const float ActualDamageToApply = CalculateReducedDamage(-OriginalDamage);
				SetHealth(HealthBeforeDamage - ActualDamageToApply);

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);
				Monster->PlayHitEffect(EffectTags);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		const float NewMaxHealth = GetMaxHealth();
		SetHealth(NewMaxHealth);
	}
}

// RepNotify 함수들 구현
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }