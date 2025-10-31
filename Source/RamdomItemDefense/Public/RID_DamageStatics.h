// Source/RamdomItemDefense/Public/RID_DamageStatics.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RID_DamageStatics.generated.h"

class UAbilitySystemComponent;
class UMyAttributeSet;
class AActor;

/**
 * @brief 치명타 데미지 텍스트를 표시하기 위한 델리게이트
 * @param TargetActor 데미지를 입은 대상 액터
 * @param CritDamageAmount 실제 적용된 치명타 데미지 양
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCritDamageOccurredDelegate, AActor*, TargetActor, float, CritDamageAmount);

/**
 * @brief 데미지 계산 관련 유틸리티 함수를 모아둔 스태틱 클래스
 */
UCLASS()
class RAMDOMITEMDEFENSE_API URID_DamageStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief (단일 대상용) 기본 데미지를 기반으로 치명타를 계산/적용/방송합니다.
	 * @param BaseDamage 치명타가 적용되기 전의 기본 데미지 (양수)
	 * @param SourceASC 공격자의 AbilitySystemComponent
	 * @param TargetActor 데미지를 입은 대상 액터 (치명타 텍스트 표시용)
	 * @param bIsSkillAttack 이 공격이 스킬인지 여부
	 * @return 치명타가 적용된 최종 데미지 (양수)
	 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static float ApplyCritDamage(float BaseDamage, UAbilitySystemComponent* SourceASC, AActor* TargetActor, bool bIsSkillAttack);

	// --- [코드 추가] ---
	/**
	 * @brief (범위 스킬용) 치명타가 발생했는지 굴림을 수행합니다.
	 * @param SourceASC 공격자의 ASC
	 * @param bIsSkillAttack 스킬 공격 여부 (스킬 확률 적용)
	 * @return 치명타 발생 시 true
	 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static bool CheckForCrit(UAbilitySystemComponent* SourceASC, bool bIsSkillAttack);

	/**
	 * @brief (범위 스킬용) 현재 스탯 기준 치명타 배율을 계산합니다.
	 * @param SourceASC 공격자의 ASC
	 * @return 치명타 배율 (예: 2.3)
	 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static float GetCritMultiplier(UAbilitySystemComponent* SourceASC);
	// --- [코드 추가 끝] ---

	/** 치명타 발생 시 방송되는 공용 델리게이트 */
	static FOnCritDamageOccurredDelegate OnCritDamageOccurred;

private:
	static const UMyAttributeSet* GetAttributeSetFromASC(UAbilitySystemComponent* SourceASC);
};