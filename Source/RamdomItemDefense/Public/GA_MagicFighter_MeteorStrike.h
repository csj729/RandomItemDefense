#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_MeteorStrike.generated.h"

class UParticleSystem;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_MeteorStrike : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_MeteorStrike();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	/** 낙하 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float FallDuration;

	/** 폭발 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ExplosionRadius;

	/** 스폰 높이 (Z축) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float SpawnHeight;

	/** 스턴 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> FallingEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

private:
	// --- [ Internal State ] ---
	FTimerHandle ImpactTimerHandle;
	FVector ImpactLocation;

	UFUNCTION()
	void Explode();
};