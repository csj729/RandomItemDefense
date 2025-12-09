// Source/RamdomItemDefense/Public/GA_BasicAttack.h (수정)

#pragma once

#include "RamdomItemDefense.h" // 프로젝트 헤더 먼저
#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GameplayTagContainer.h"
#include "GA_BasicAttack.generated.h"

class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_BasicAttack : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_BasicAttack();

protected:
	/** 어빌리티 활성화 시 실행 (공격 및 디버프 적용) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 슬로우 효과 적용 (Percent: 0.2 -> 20%) */
	void ApplySlowEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float SlowPercent, float Duration);

	/** 스턴 효과 적용 */
	void ApplyStunEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Duration);

	/** 방어력 감소 효과 적용 (ArmorAmount: 깎을 수치) */
	void ApplyArmorShredEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float ArmorAmount, float Duration);

	/** (기존) 데미지 수치 전달용 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag DamageDataTag;

	/** 적용할 스턴 이펙트 클래스 (GE_Stun 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Effects")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** 적용할 슬로우 이펙트 클래스 (GE_Slow 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Effects")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	/** 적용할 방어력 감소 이펙트 클래스 (GE_ArmorShred 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Effects")
	TSubclassOf<UGameplayEffect> ArmorReductionEffectClass;

	/** 슬로우 수치를 전달할 SetByCaller 태그 (기본값: Debuff.Slow.Amount) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag SlowAmountTag;

	/** 방어력 감소 수치를 전달할 SetByCaller 태그 (기본값: Debuff.ArmorShred.Amount) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag ArmorShredAmountTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag DurationTag;
};