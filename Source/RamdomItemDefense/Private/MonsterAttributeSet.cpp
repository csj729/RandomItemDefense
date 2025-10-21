// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"

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
		// --- [�ڵ� ����] ---
		// ������ ���� *��*�� ���� ü�� ���� �����ɴϴ�.
		const float NewHealth = GetHealth();

		// 1. ���� ���͸� ���� �����ɴϴ�.
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		// 2. ���Ͱ� ��ȿ�ϰ�, [�ٽ�] ���� ���� ����(bIsDying)�� �ƴ��� Ȯ���մϴ�.
		if (Monster && !Monster->IsDying())
		{
			// 3. �� �������� ���� ü���� 0 ���ϰ� �Ǿ����� Ȯ���մϴ�.
			if (NewHealth <= 0.f)
			{
				// ų��(Instigator/EffectCauser)�� ã���ϴ�.
				AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
				AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();

				ARamdomItemDefenseCharacter* KillerCharacter = Cast<ARamdomItemDefenseCharacter>(InstigatorActor);
				if (KillerCharacter == nullptr)
				{
					KillerCharacter = Cast<ARamdomItemDefenseCharacter>(EffectCauserActor);
				}

				// (����� �α״� �����ϼŵ� �����ϴ�)

				// 4. Die() �Լ��� ȣ���մϴ�. (�� �Լ� ���ο��� bIsDying�� true�� �����˴ϴ�)
				Monster->Die(KillerCharacter);

				// 5. ų���� ��ȿ�ϸ� ��带 �����մϴ�.
				if (KillerCharacter)
				{
					AMyPlayerState* PS = KillerCharacter->GetPlayerState<AMyPlayerState>();
					if (PS)
					{
						const int32 GoldAmount = Monster->GetGoldOnDeath();
						PS->AddGold(GoldAmount);

						if (GEngine)
						{
							FString GoldMessage = FString::Printf(TEXT("Awarded %d gold to %s for killing %s"),
								GoldAmount,
								*PS->GetPlayerName(),
								*Monster->GetName()
							);
							GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, GoldMessage);
						}
					}
				}
				// (ų�� ������ ���� ����� �α״� �����ϼŵ� �����ϴ�)
			}
		}
		// ------------------
	}
}

// RepNotify �Լ��� ����
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }