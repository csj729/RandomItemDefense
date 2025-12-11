#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Soldier_Grenade.generated.h"

class UGameplayEffect;
class UParticleSystem;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_Grenade : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Soldier_Grenade();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FVector ImpactEffectScale;
};