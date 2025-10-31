// Source/RamdomItemDefense/Private/RID_DamageStatics.cpp (����)

#include "RID_DamageStatics.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h" 
#include "RamdomItemDefense.h"
#include "GameFramework/Actor.h" 
// ĳ������ GetAttributeSet()�� �����ϱ� ���� 2���� ��� �߰�
#include "AbilitySystemInterface.h"
#include "RamdomItemDefenseCharacter.h" 


// Static ��������Ʈ ���� ����
FOnCritDamageOccurredDelegate URID_DamageStatics::OnCritDamageOccurred;

/** ASC���� MyAttributeSet�� �����ϰ� �����ɴϴ�. */
const UMyAttributeSet* URID_DamageStatics::GetAttributeSetFromASC(UAbilitySystemComponent* SourceASC)
{
	if (!SourceASC) return nullptr;

	// ��� 1: ASC�� ������(ĳ����)�� ���� ���� AttributeSet ��������
	if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(SourceASC->GetOwnerActor()))
	{
		if (OwnerCharacter->GetAttributeSet())
		{
			// UE_LOG(LogRamdomItemDefense, Log, TEXT("GetAttributeSetFromASC: Found via GetOwnerActor() method."));
			return OwnerCharacter->GetAttributeSet();
		}
	}
	// --- [ �ڡڡ� ���� �ڡڡ� ] ---
	UE_LOG(LogRamdomItemDefense, Error, TEXT("GetAttributeSetFromASC: FAILED to find UMyAttributeSet using ANY method."));
	// --- [ �ڡڡ� ���� �� �ڡڡ� ] ---
	return nullptr;
}

/** (���� ��ų��) ġ��Ÿ ���� ��� */
float URID_DamageStatics::GetCritMultiplier(UAbilitySystemComponent* SourceASC)
{
	const UMyAttributeSet* AttributeSet = GetAttributeSetFromASC(SourceASC);
	if (!AttributeSet)
	{
		// --- [ �ڡڡ� ���� �ڡڡ� ] ---
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GetCritMultiplier: AttributeSet is NULL. Returning base 2.0x multiplier."));
		// --- [ �ڡڡ� ���� �� �ڡڡ� ] ---
		return 2.0f; // �Ӽ��� ������ �⺻ 2��
	}

	const float BonusCritDamageValue = AttributeSet->GetCritDamage();
	const float TotalCritMultiplier = 2.0f + BonusCritDamageValue;

	// --- [ �ڡڡ� ���� �ڡڡ� ] ---
	UE_LOG(LogRamdomItemDefense, Log, TEXT("GetCritMultiplier: BonusCritDamageValue=%.2f, TotalMultiplier=%.2fx"),
		BonusCritDamageValue,
		TotalCritMultiplier);
	// --- [ �ڡڡ� ���� �� �ڡڡ� ] ---

	return TotalCritMultiplier;
}

/** (���� ��ų��) ġ��Ÿ ���� */
bool URID_DamageStatics::CheckForCrit(UAbilitySystemComponent* SourceASC, bool bIsSkillAttack)
{
	const UMyAttributeSet* AttributeSet = GetAttributeSetFromASC(SourceASC);
	if (!AttributeSet)
	{
		return false;
	}

	const float BaseCritChance = AttributeSet->GetCritChance();
	const float ActualCritChance = bIsSkillAttack ? (BaseCritChance / 5.0f) : BaseCritChance;

	return (FMath::FRand() < ActualCritChance);
}

/** (���� ����) ġ��Ÿ ���/����/��� */
float URID_DamageStatics::ApplyCritDamage(float BaseDamage, UAbilitySystemComponent* SourceASC, AActor* TargetActor, bool bIsSkillAttack)
{
	if (!SourceASC || BaseDamage <= 0.f)
	{
		return BaseDamage;
	}

	// ������ ����� �и��� �Լ��� ����
	if (CheckForCrit(SourceASC, bIsSkillAttack))
	{
		// --- ġ��Ÿ ���� ---
		const float TotalCritMultiplier = GetCritMultiplier(SourceASC); // ������ �Լ� ȣ��
		const float CritDamageAmount = BaseDamage * TotalCritMultiplier;

		// ��������Ʈ ��� (TargetActor�� ��ȿ�� ����)
		if (TargetActor)
		{
			OnCritDamageOccurred.Broadcast(TargetActor, CritDamageAmount);
		}

		// --- [ �ڡڡ� ���� �ڡڡ� ] ---
		// (ġ��Ÿ�� ������ �ƴϹǷ� Log�� ����)
		UE_LOG(LogRamdomItemDefense, Log, TEXT("CRITICAL HIT! (%.1f * %.2fx) -> %.1f"), BaseDamage, TotalCritMultiplier, CritDamageAmount);
		// --- [ �ڡڡ� ���� �� �ڡڡ� ] ---
		return CritDamageAmount;
	}
	else
	{
		// --- ġ��Ÿ ���� ---
		return BaseDamage;
	}
}