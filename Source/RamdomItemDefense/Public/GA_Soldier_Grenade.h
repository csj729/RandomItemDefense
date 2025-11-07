// Public/GA_Soldier_Grenade.h

#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Soldier_Grenade.generated.h"

class UGameplayEffect;
class UParticleSystem; // (수정) UNiagaraSystem -> UParticleSystem

/**
 * 솔져 스킬 1: 유탄 발사
 * 타겟 위치 300 Radius에 광역 데미지와 슬로우를 적용합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_Grenade : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Soldier_Grenade();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (BP 설정) 폭발 반경 (기획: 300) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float ExplosionRadius;

	/** (BP 설정) 적용할 슬로우 GameplayEffect (예: GE_Slow_..._...s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	// --- [ ★★★ 수정 ★★★ ] ---
	/** (BP 설정) 스킬 발동 시 타겟 위치에 스폰할 폭발 이펙트 (캐스케이드) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ImpactEffect;
	// --- [ ★★★ 수정 끝 ★★★ ] ---

	/** (BP 설정) 이펙트 스케일 조정 (반경 300에 맞게) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FVector ImpactEffectScale;
};