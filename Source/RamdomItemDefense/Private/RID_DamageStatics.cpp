// Source/RamdomItemDefense/Private/RID_DamageStatics.cpp (수정)

#include "RID_DamageStatics.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h" 
#include "RamdomItemDefense.h"
#include "GameFramework/Actor.h" 
// 캐릭터의 GetAttributeSet()에 접근하기 위해 2개의 헤더 추가
#include "AbilitySystemInterface.h"
#include "RamdomItemDefenseCharacter.h" 


// Static 델리게이트 변수 정의
FOnCritDamageOccurredDelegate URID_DamageStatics::OnCritDamageOccurred;

/** ASC에서 MyAttributeSet을 안전하게 가져옵니다. */
const UMyAttributeSet* URID_DamageStatics::GetAttributeSetFromASC(UAbilitySystemComponent* SourceASC)
{
	if (!SourceASC) return nullptr;

	// 방법 1: ASC의 소유자(캐릭터)를 통해 직접 AttributeSet 가져오기
	if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(SourceASC->GetOwnerActor()))
	{
		if (OwnerCharacter->GetAttributeSet())
		{
			// UE_LOG(LogRamdomItemDefense, Log, TEXT("GetAttributeSetFromASC: Found via GetOwnerActor() method."));
			return OwnerCharacter->GetAttributeSet();
		}
	}
	// --- [ ★★★ 수정 ★★★ ] ---
	UE_LOG(LogRamdomItemDefense, Error, TEXT("GetAttributeSetFromASC: FAILED to find UMyAttributeSet using ANY method."));
	// --- [ ★★★ 수정 끝 ★★★ ] ---
	return nullptr;
}

/** (범위 스킬용) 치명타 배율 계산 */
float URID_DamageStatics::GetCritMultiplier(UAbilitySystemComponent* SourceASC)
{
	const UMyAttributeSet* AttributeSet = GetAttributeSetFromASC(SourceASC);
	if (!AttributeSet)
	{
		// --- [ ★★★ 수정 ★★★ ] ---
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GetCritMultiplier: AttributeSet is NULL. Returning base 2.0x multiplier."));
		// --- [ ★★★ 수정 끝 ★★★ ] ---
		return 2.0f; // 속성셋 없으면 기본 2배
	}

	const float BonusCritDamageValue = AttributeSet->GetCritDamage();
	const float TotalCritMultiplier = 2.0f + BonusCritDamageValue;

	// --- [ ★★★ 수정 ★★★ ] ---
	UE_LOG(LogRamdomItemDefense, Log, TEXT("GetCritMultiplier: BonusCritDamageValue=%.2f, TotalMultiplier=%.2fx"),
		BonusCritDamageValue,
		TotalCritMultiplier);
	// --- [ ★★★ 수정 끝 ★★★ ] ---

	return TotalCritMultiplier;
}

/** (범위 스킬용) 치명타 굴림 */
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

/** (단일 대상용) 치명타 계산/적용/방송 */
float URID_DamageStatics::ApplyCritDamage(float BaseDamage, UAbilitySystemComponent* SourceASC, AActor* TargetActor, bool bIsSkillAttack)
{
	if (!SourceASC || BaseDamage <= 0.f)
	{
		return BaseDamage;
	}

	// 굴림과 계산을 분리된 함수로 수행
	if (CheckForCrit(SourceASC, bIsSkillAttack))
	{
		// --- 치명타 성공 ---
		const float TotalCritMultiplier = GetCritMultiplier(SourceASC); // 수정된 함수 호출
		const float CritDamageAmount = BaseDamage * TotalCritMultiplier;

		// 델리게이트 방송 (TargetActor가 유효할 때만)
		if (TargetActor)
		{
			OnCritDamageOccurred.Broadcast(TargetActor, CritDamageAmount);
		}

		// --- [ ★★★ 수정 ★★★ ] ---
		// (치명타는 에러가 아니므로 Log로 변경)
		UE_LOG(LogRamdomItemDefense, Log, TEXT("CRITICAL HIT! (%.1f * %.2fx) -> %.1f"), BaseDamage, TotalCritMultiplier, CritDamageAmount);
		// --- [ ★★★ 수정 끝 ★★★ ] ---
		return CritDamageAmount;
	}
	else
	{
		// --- 치명타 실패 ---
		return BaseDamage;
	}
}