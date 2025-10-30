// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"
#include "RamdomItemDefense.h" // RID_LOG ��ũ�ο�

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

	// [�α� 1: �Լ� ���� Ȯ��]
	RID_LOG(FColor::White, TEXT("MonsterAttributeSet: PostGameplayEffectExecute CALLED. Attribute: %s"), *Data.EvaluatedData.Attribute.GetName());

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// [�α� 2: ü�� �Ӽ� ���� Ȯ��]
		RID_LOG(FColor::Cyan, TEXT("MonsterAttributeSet: Health Attribute CHANGED."));

		const float NewHealth = GetHealth();
		const float Magnitude = Data.EvaluatedData.Magnitude; // ������/����

		// [�α� 3: �� �� Ȯ��]
		RID_LOG(FColor::Cyan, TEXT("MonsterAttributeSet: NewHealth: %.1f, Magnitude: %.1f"), NewHealth, Magnitude);

		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		if (Monster && !Monster->IsDying())
		{
			if (NewHealth <= 0.f)
			{
				// [�α� 4: ��� ó�� ����]
				RID_LOG(FColor::Red, TEXT("MonsterAttributeSet: Monster is DYING (NewHealth <= 0)."));

				// (���� ��� ����)
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

				// --- [ �ڡڡ� �ڵ� �߰� �ڡڡ� ] ---
				// ���Ͱ� �״� �������� �ǰ� ����Ʈ�� ����մϴ�.
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
				// --- [ �ڵ� �߰� �� ] ---
			}
			else if (Magnitude < 0.f)
			{
				// [�α� 5: �ǰ� ó�� ����]
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
				// [�α� 6: �ƹ� ���ǵ� ���� ����]
				RID_LOG(FColor::Orange, TEXT("MonsterAttributeSet: Health changed, but NOT hit (Magnitude: %.1f) and NOT dead (NewHealth: %.1f)."), Magnitude, NewHealth);
			}
		}
	}
}

// RepNotify �Լ��� ����
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }