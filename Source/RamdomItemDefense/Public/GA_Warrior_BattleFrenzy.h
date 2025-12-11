#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Warrior_BattleFrenzy.generated.h"

class UGameplayEffect;
class UParticleSystem;
class UParticleSystemComponent;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Warrior_BattleFrenzy : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Warrior_BattleFrenzy();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- [ Configuration ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> AttackSpeedBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float BuffDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float AttackSpeedCoefficient;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag AttackSpeedBuffTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag BuffIsActiveTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FName AttachSocketName;

private:
	// --- [ Internal State ] ---
	FActiveGameplayEffectHandle ActiveBuffEffectHandle;

	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> ActiveBuffFXComponent;

	UFUNCTION()
	void OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo);
};