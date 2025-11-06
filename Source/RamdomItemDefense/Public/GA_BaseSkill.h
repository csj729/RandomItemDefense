// Public/GA_BaseSkill.h (수정)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_BaseSkill.generated.h"

class UGameplayEffect;

/**
 * @brief 모든 확률 발동형 스킬의 부모 클래스.
 * 공통 속성(발동 확률, 데미지 GE 등)을 정의합니다.
 */
UCLASS(Abstract) // 이 클래스로 직접 BP를 만들지 못하도록 '추상' 클래스로 선언
class RAMDOMITEMDEFENSE_API UGA_BaseSkill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BaseSkill();

	/**
	 * @brief 이 스킬의 기본 발동 확률 (0.0 ~ 1.0)
	 * GA_AttackSelecter가 이 값을 읽어와서 확률 계산에 사용합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config")
	float BaseActivationChance;

	/**
	 * @brief 이 스킬이 적용할 데미지 GE
	 * (SetByCaller를 사용할 수도 있고, 아닐 수도 있으므로 Base에서는 TSubclassOf만 제공)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/**
	 * @brief 데미지 GE에 SetByCaller로 값을 전달할 때 사용할 태그
	 * (필요 없는 스킬은 비워두면 됨)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|GAS")
	FGameplayTag DamageByCallerTag;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	/**
	 * @brief (BP Class Default에서 설정) 이 스킬의 기본 데미지
	 * (예: 100)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|Damage")
	float DamageBase;

	/**
	 * @brief (BP Class Default에서 설정) 이 스킬의 공격력 계수
	 * (예: 1.5 = 공격력의 150%)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|Damage")
	float DamageCoefficient;
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
};