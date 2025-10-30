#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h" // FGameplayTag 사용을 위해 추가
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_MeteorStrike.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_MeteorStrike : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_MeteorStrike();

	/** GA_AttackSelecter가 이 어빌리티를 호출할 때 실행됩니다. */
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	/** 메테오가 떨어지는 데 걸리는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float FallDuration = 0.1f;

	/** 폭발 반경 (기획: 500) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ExplosionRadius = 500.0f;

	/** 타겟 위치에서 얼마나 위에서 소환할지 (Z축) - 이펙트 스폰용 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float SpawnHeight = 2000.0f;

	/** (Stun GE) 폭발 시 적용할 2초 스턴 GameplayEffect (블루프린트에서 GE_Stun_2s 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** 하늘에서 떨어지는 메테오 파티클 (Velocity가 설정된 파티클) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> FallingEffect;

	/** 바닥에서 폭발하는 파티클 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

private:
	/** FallDuration 이후 Explode 함수를 호출하기 위한 타이머 핸들 */
	FTimerHandle ImpactTimerHandle;

	/** 폭발 지점 (타겟의 초기 위치) */
	UPROPERTY()
	FVector ImpactLocation;

	/** 타이머가 만료되었을 때 실제 폭발을 실행하는 함수 */
	UFUNCTION()
	void Explode();
};