// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"
#include "RamdomItemDefense.h"
#include "MyGameState.h"       // GetCurrentWave() ���
#include "Engine/World.h"      // GetWorld() ���

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

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float NewHealth = GetHealth();
		const float Magnitude = Data.EvaluatedData.Magnitude; // ������/����

		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		if (Monster && !Monster->IsDying())
		{
			if (NewHealth <= 0.f)
			{
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
						// --- [ �ڡڡ� ���� óġ ��� ��� ���� ���� �ڡڡ� ] ---

						// 1. GameState���� ���� ���̺� ��������
						AMyGameState* MyGameState = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
						int32 CurrentWave = 1; // GameState�� ���ų� ���̺갡 0�̸� 1�� ����
						if (MyGameState && MyGameState->GetCurrentWave() > 0)
						{
							CurrentWave = MyGameState->GetCurrentWave();
						}

						// 2. ���̺� ������� ��� ��� (10 + (Wave - 1) * 5)
						// const int32 GoldAmount = Monster->GetGoldOnDeath(); // (���� �ڵ�)
						const int32 BaseGold = 10;
						const int32 BonusGold = (CurrentWave - 1) * 5;
						const int32 GoldAmount = FMath::Max(BaseGold, BaseGold + BonusGold); // Wave 1�� �� 10��� ����

						// --- [ �ڡڡ� ���� ���� �� �ڡڡ� ] ---

						PS->AddGold(GoldAmount);
						RID_LOG(FColor::Yellow, TEXT("Awarded %d gold to %s for killing %s (Wave: %d)"),
							GoldAmount,
							*PS->GetPlayerName(),
							*Monster->GetName(),
							CurrentWave
						);
					}
				}

				// --- [ �ǰ� ����Ʈ ��� (��� ��) ] ---
				RID_LOG(FColor::Yellow, TEXT("MonsterAttributeSet: Playing HIT effect on DEATH. Checking Asset Tags..."));

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);

				Monster->PlayHitEffect(EffectTags);
				// --- [ �ڵ� �� ] ---
			}
			else if (Magnitude < 0.f)
			{
				// [�α� 5: �ǰ� ó�� ����]
				//RID_LOG(FColor::Yellow, TEXT("MonsterAttributeSet: Monster was HIT (Magnitude < 0 and NewHealth > 0). Checking Asset Tags..."));

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);


				Monster->PlayHitEffect(EffectTags);
			}
			else
			{
				// [�α� 6: �ƹ� ���ǵ� ���� ����]
				RID_LOG(FColor::Orange, TEXT("MonsterAttributeSet: Health changed, but NOT hit (Magnitude: %.1f) and NOT dead (NewHealth: %.1f)."), Magnitude, NewHealth);
			}
		}
	}
	// --- [ Health/MaxHealth ����ȭ ���� ] ---
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// [�α� 7: MaxHealth ���� ����]
		//RID_LOG(FColor::Green, TEXT("MonsterAttributeSet: MaxHealth Attribute CHANGED."));

		// �� MaxHealth ���� �����ɴϴ�.
		const float NewMaxHealth = GetMaxHealth();

		// ���� Health ���� �� MaxHealth ������ ��� �����մϴ�.
		SetHealth(NewMaxHealth);

		// [�α� 8: Health ����ȭ �Ϸ�]
		//RID_LOG(FColor::Green, TEXT("MonsterAttributeSet: Health SYNCED to NewMaxHealth: %.1f"), NewMaxHealth);
	}
	// --- [ ���� �� ] ---
}

// RepNotify �Լ��� ����
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }