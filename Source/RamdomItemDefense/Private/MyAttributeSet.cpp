// MyAttributeSet.cpp

#include "MyAttributeSet.h"
#include "Net/UnrealNetwork.h"

// �� �Լ� �ȿ� ������ ��� ������ ����ؾ� �մϴ�.
void UMyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // �⺻ ����
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, AttackDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, CritDamage, COND_None, REPNOTIFY_Always);

    // Ư�� ����
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, ArmorReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, MoveSpeedReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, SkillActivationChance, COND_None, REPNOTIFY_Always);

    // ������ ���� ����
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, CritChance, COND_None, REPNOTIFY_Always);

    // ���� ����
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);

}

// �� ������ OnRep �Լ� ���� (���� ����Ǿ��� �� Ŭ���̾�Ʈ���� ȣ���)

void UMyAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, AttackDamage, OldValue);
}

void UMyAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, AttackSpeed, OldValue);
}

void UMyAttributeSet::OnRep_CritDamage(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, CritDamage, OldValue);
}

void UMyAttributeSet::OnRep_ArmorReduction(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, ArmorReduction, OldValue);
}

void UMyAttributeSet::OnRep_MoveSpeedReduction(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, MoveSpeedReduction, OldValue);
}

void UMyAttributeSet::OnRep_SkillActivationChance(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, SkillActivationChance, OldValue);
}

void UMyAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, CritChance, OldValue);
}

void UMyAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, AttackRange, OldValue);
}