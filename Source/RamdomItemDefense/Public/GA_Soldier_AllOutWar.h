#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_Soldier_AllOutWar.generated.h"

class UGameplayEffect;
class UParticleSystem;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_AllOutWar : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_Soldier_AllOutWar();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- [ Configuration ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> AllOutWarBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag AttackSpeedBuffTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag BuffIsActiveTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FName BuffEffectAttachSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FVector BuffEffectScale;

private:
	// --- [ Internal State ] ---
	FActiveGameplayEffectHandle UltimateBuffEffectHandle;

	UFUNCTION()
	void OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo);
};