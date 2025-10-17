// MyAttributeSet.cpp

#include "MyAttributeSet.h"
#include "Net/UnrealNetwork.h"

// 이 함수 안에 복제할 모든 스탯을 등록해야 합니다.
void UMyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 기본 스탯
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, AttackDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, CritDamage, COND_None, REPNOTIFY_Always);

    // 특수 스탯
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, ArmorReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, MoveSpeedReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, SkillActivationChance, COND_None, REPNOTIFY_Always);

    // 아이템 전용 스탯
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, CritChance, COND_None, REPNOTIFY_Always);

    // 고정 스탯
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);

}

// 각 스탯의 OnRep 함수 구현 (값이 변경되었을 때 클라이언트에서 호출됨)

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