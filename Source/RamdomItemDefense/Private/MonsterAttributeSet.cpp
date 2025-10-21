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
        const float OldHealth = GetHealth();
        // NewHealth�� �̹� ���� ���� ���̹Ƿ� GetHealth() ��� ����մϴ�.
        const float NewHealth = Data.EvaluatedData.Magnitude + OldHealth;

        // OldHealth > 0 �������� "�̹� ���� �ʾҴ���" �ݵ�� Ȯ���մϴ�.
        if (NewHealth <= 0.f)
        {
            AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

            // --- [�ڵ� ����] ---
            // Instigator (������)�� EffectCauser (ȿ�� ������)�� ��� �����ɴϴ�.
            AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
            AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();

            // 1. Instigator�� ĳ������ ���ϴ�.
            ARamdomItemDefenseCharacter* KillerCharacter = Cast<ARamdomItemDefenseCharacter>(InstigatorActor);

            // 2. ���� Instigator�� �÷��̾� ĳ���Ͱ� �ƴ϶�� (��: PlayerController),
            //    ��� �������Ʈ���� ������ EffectCauser�� ĳ������ ���ϴ�.
            if (KillerCharacter == nullptr)
            {
                KillerCharacter = Cast<ARamdomItemDefenseCharacter>(EffectCauserActor);
            }
            // ------------------

            // (������� ���� �� ������ �̸��� ��� ����غ��ϴ�)
            if (GEngine)
            {
                FString InstigatorName = (InstigatorActor) ? InstigatorActor->GetName() : TEXT("NULL (Instigator)");
                FString CauserName = (EffectCauserActor) ? EffectCauserActor->GetName() : TEXT("NULL (EffectCauser)");
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Instigator: %s"), *InstigatorName));
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("EffectCauser: %s"), *CauserName));
            }


            // 1. ������ ���� ó���� ���� ȣ���մϴ�.
            if (Monster)
            {
                Monster->Die(KillerCharacter);
            }

            // 2. ų���� ���Ͱ� ��� ��ȿ�� ��쿡�� ��带 �����մϴ�.
            if (KillerCharacter && Monster)
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
            else if (GEngine) // ų�� ������ ������ �� ������ ��� ����� �޽���
            {
                FString MonsterName = (Monster) ? Monster->GetName() : TEXT("NULL");
                FString KillerName = (KillerCharacter) ? KillerCharacter->GetName() : TEXT("NULL");
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                    FString::Printf(TEXT("Gold Award FAILED. Killer: %s, Monster: %s"), *KillerName, *MonsterName));
            }
        }
    }
}
// RepNotify �Լ��� ����
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }