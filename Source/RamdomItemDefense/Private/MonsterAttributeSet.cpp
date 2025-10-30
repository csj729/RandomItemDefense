// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"
#include "RamdomItemDefense.h" // RID_LOG 매크로용

void UMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// [로그 1: 함수 진입 확인]
	RID_LOG(FColor::White, TEXT("MonsterAttributeSet: PostGameplayEffectExecute CALLED. Attribute: %s"), *Data.EvaluatedData.Attribute.GetName());

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// [로그 2: 체력 속성 진입 확인]
		RID_LOG(FColor::Cyan, TEXT("MonsterAttributeSet: Health Attribute CHANGED."));

		const float NewHealth = GetHealth();
		const float Magnitude = Data.EvaluatedData.Magnitude; // 데미지/힐량

		// [로그 3: 상세 값 확인]
		RID_LOG(FColor::Cyan, TEXT("MonsterAttributeSet: NewHealth: %.1f, Magnitude: %.1f"), NewHealth, Magnitude);

		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		if (Monster && !Monster->IsDying())
		{
			if (NewHealth <= 0.f)
			{
				// [로그 4: 사망 처리 진입]
				RID_LOG(FColor::Red, TEXT("MonsterAttributeSet: Monster is DYING (NewHealth <= 0)."));

				// (기존 사망 로직)
				AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
				AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();

				ARamdomItemDefenseCharacter* KillerCharacter = Cast<ARamdomItemDefenseCharacter>(InstigatorActor);
				if (KillerCharacter == nullptr)
				{
					KillerCharacter = Cast<ARamdomItemDefenseCharacter>(EffectCauserActor);
				}

				Monster->Die(KillerCharacter);

				if (KillerCharacter)
				{
					AMyPlayerState* PS = KillerCharacter->GetPlayerState<AMyPlayerState>();
					if (PS)
					{
						const int32 GoldAmount = Monster->GetGoldOnDeath();
						PS->AddGold(GoldAmount);
						RID_LOG(FColor::Yellow, TEXT("Awarded %d gold to %s for killing %s"),
							GoldAmount,
							*PS->GetPlayerName(),
							*Monster->GetName()
						);
					}
				}

				// --- [ ★★★ 코드 추가 ★★★ ] ---
				// 몬스터가 죽는 순간에도 피격 이펙트를 재생합니다.
				RID_LOG(FColor::Yellow, TEXT("MonsterAttributeSet: Playing HIT effect on DEATH. Checking Asset Tags..."));

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);

				if (EffectTags.Num() > 0)
				{
					RID_LOG(FColor::Green, TEXT("MonsterAttributeSet: Death Hit! Tags found: %s"), *EffectTags.ToString());
				}
				else
				{
					RID_LOG(FColor::Red, TEXT("MonsterAttributeSet: Death Hit! BUT NO ASSET TAGS FOUND ON GE!"));
				}

				Monster->PlayHitEffect(EffectTags);
				// --- [ 코드 추가 끝 ] ---
			}
			else if (Magnitude < 0.f)
			{
				// [로그 5: 피격 처리 진입]
				RID_LOG(FColor::Yellow, TEXT("MonsterAttributeSet: Monster was HIT (Magnitude < 0 and NewHealth > 0). Checking Asset Tags..."));

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);

				if (EffectTags.Num() > 0)
				{
					RID_LOG(FColor::Green, TEXT("MonsterAttributeSet: Hit Detected! Tags found: %s"), *EffectTags.ToString());
				}
				else
				{
					RID_LOG(FColor::Red, TEXT("MonsterAttributeSet: Hit Detected! BUT NO ASSET TAGS FOUND ON GE!"));
				}

				Monster->PlayHitEffect(EffectTags);
			}
			else
			{
				// [로그 6: 아무 조건도 맞지 않음]
				RID_LOG(FColor::Orange, TEXT("MonsterAttributeSet: Health changed, but NOT hit (Magnitude: %.1f) and NOT dead (NewHealth: %.1f)."), Magnitude, NewHealth);
			}
		}
	}
}

// RepNotify 함수들 구현
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }