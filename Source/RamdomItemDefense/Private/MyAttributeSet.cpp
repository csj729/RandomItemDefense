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
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, StunChance, COND_None, REPNOTIFY_Always);

    // 캐릭터 고정 스탯
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

void UMyAttributeSet::OnRep_StunChance(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, StunChance, OldValue);
}

void UMyAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, AttackRange, OldValue);
}

// --- BaseValue 조정 함수 구현 ---
void UMyAttributeSet::AdjustBaseAttackDamage(float Delta)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (ASC && ASC->IsOwnerActorAuthoritative() && FMath::Abs(Delta) > SMALL_NUMBER)
	{
		// --- [코드 수정] ---
		// GetBaseAttackDamage() 대신 AttackDamage 변수의 GetBaseValue() 호출
		ASC->SetNumericAttributeBase(GetAttackDamageAttribute(), AttackDamage.GetBaseValue() + Delta);
		// --- ----------- ---
	}
}

void UMyAttributeSet::AdjustBaseAttackSpeed(float Delta)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (ASC && ASC->IsOwnerActorAuthoritative() && FMath::Abs(Delta) > SMALL_NUMBER)
	{
		// --- [코드 수정] ---
		ASC->SetNumericAttributeBase(GetAttackSpeedAttribute(), AttackSpeed.GetBaseValue() + Delta);
		// --- ----------- ---
	}
}

void UMyAttributeSet::AdjustBaseCritDamage(float Delta)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (ASC && ASC->IsOwnerActorAuthoritative() && FMath::Abs(Delta) > SMALL_NUMBER)
	{
		// --- [코드 수정] ---
		ASC->SetNumericAttributeBase(GetCritDamageAttribute(), CritDamage.GetBaseValue() + Delta * 100.f);
		// --- ----------- ---
	}
}

void UMyAttributeSet::AdjustBaseArmorReduction(float Delta)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (ASC && ASC->IsOwnerActorAuthoritative() && FMath::Abs(Delta) > SMALL_NUMBER)
	{
		// --- [코드 수정] ---
		ASC->SetNumericAttributeBase(GetArmorReductionAttribute(), ArmorReduction.GetBaseValue() + Delta);
		// --- ----------- ---
	}
}

void UMyAttributeSet::AdjustBaseSkillActivationChance(float Delta)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (ASC && ASC->IsOwnerActorAuthoritative() && FMath::Abs(Delta) > SMALL_NUMBER)
	{
		// --- [코드 수정] ---
		ASC->SetNumericAttributeBase(GetSkillActivationChanceAttribute(), SkillActivationChance.GetBaseValue() + Delta);
		// --- ----------- ---
	}
}