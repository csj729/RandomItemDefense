// Public/GA_Warrior_Shockwave.h

#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Warrior_Shockwave.generated.h"

class UGameplayEffect;
class UParticleSystem;

/**
 * 전사 스킬 3: 충격파
 * 타겟 주변 일정 범위의 적들에게 데미지를 주고 3초 스턴을 적용합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Warrior_Shockwave : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Warrior_Shockwave();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (BP 설정) 폭발 반경 (예: 400) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float ExplosionRadius;

	/** (BP 설정) 적용할 3초 스턴 GameplayEffect (예: GE_Stun_3s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** (BP 설정) 스킬 발동 시 타겟 위치에 스폰할 이펙트 (예: P_Shockwave_Impact) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ImpactEffect;
};