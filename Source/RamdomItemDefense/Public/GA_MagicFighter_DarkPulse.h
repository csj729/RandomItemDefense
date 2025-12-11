#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_DarkPulse.generated.h"

class UParticleSystem;
class USoundBase;
class AProjectileBase;
class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_DarkPulse : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_DarkPulse();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	float VisualProjectileSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Projectile")
	FName ProjectileSpawnSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Explosion")
	float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

private:
	// --- [ Internal State ] ---
	FTimerHandle ImpactTimerHandle;
	FVector TargetImpactLocation;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UFUNCTION()
	void Explode();
};