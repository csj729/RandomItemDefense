// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"

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
        // AttributeSet�� ���� ü�� ���� �����ɴϴ�. �̰��� '���� ��'�Դϴ�.
        const float OldHealth = GetHealth();

        // ����� ��ȭ�� (�������� ���� �����̹Ƿ� �״�� ���)
        const float Magnitude = Data.EvaluatedData.Magnitude;

        // ����Ǵ� '���ο� ��'�� ����մϴ�.
        const float NewHealth = OldHealth + Magnitude;

        if (GEngine)
        {
            FString DebugMessage = FString::Printf(TEXT("Monster Health (Server): %.1f -> %.1f (Change: %.1f)"),
                OldHealth,    // ���� ü��
                NewHealth,    // ���� ���ο� ü��
                Magnitude     // ����� ��ȭ��
            );
            // ȭ�鿡 2�ʰ� û�ϻ����� �޽����� ����մϴ�. (OnRep�� �� ����)
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, DebugMessage);
        }
    }
}

// RepNotify �Լ��� ����
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }