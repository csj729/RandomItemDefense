// Public/GA_Warrior_GroundSlam.h

#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Warrior_GroundSlam.generated.h"

class UGameplayEffect;
class UParticleSystem;

/**
 * 전사 스킬 1: 지면 강타
 * 타겟 주변 일정 범위의 적들에게 데미지를 주고 30% 슬로우를 적용합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Warrior_GroundSlam : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Warrior_GroundSlam();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (BP 설정) 폭발 반경 (예: 300) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float ExplosionRadius;

	/** (BP 설정) 적용할 30% 슬로우 GameplayEffect (예: GE_Slow_30Percent_3s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	/** (BP 설정) 스킬 발동 시 타겟 위치에 스폰할 이펙트 (예: P_GroundSlam_Impact) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ImpactEffect;
};