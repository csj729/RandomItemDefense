#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_Warrior_Warpath.generated.h"

class UGameplayEffect;
class UParticleSystem;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Warrior_Warpath : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_Warrior_Warpath();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- [ Configuration ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> UltimateStateEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> UltimateBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag DamageDataTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag BuffIsActiveTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Damage")
	float DamageRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> CasterEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FName BuffEffectAttachSocketName;

private:
	// --- [ Internal State ] ---
	FActiveGameplayEffectHandle UltimateStateEffectHandle;
	FActiveGameplayEffectHandle UltimateBuffEffectHandle;

	UFUNCTION() void OnMontageFinished();
	UFUNCTION() void OnMontageCancelled();
	UFUNCTION() void OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo);
};