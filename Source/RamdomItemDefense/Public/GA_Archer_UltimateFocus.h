#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_Archer_UltimateFocus.generated.h"

class UParticleSystem;
class USoundBase;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_UltimateFocus : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_UltimateFocus();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- [ Configuration ] ---
	/** ±Ã±Ø±â ¹öÇÁ GE */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> UltimateBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BuffDuration;

	/** È°¼ºÈ­ ½Ã Àç»ýÇÒ 1È¸¼º ÀÌÆåÆ® */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ActivationEffect;

	/** ¸öÅë ºÎÂø Áö¼Ó ÀÌÆåÆ® */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffParticleSystem;

	/** ¸Ó¸® ºÎÂø Áö¼Ó ÀÌÆåÆ® */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> HeadBuffParticleSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<USoundBase> BuffStartSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag BuffIsActiveTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag HeadBuffIsActiveTag;

private:
	// --- [ Internal State ] ---
	FActiveGameplayEffectHandle UltimateBuffEffectHandle;
	FTimerHandle BuffDurationTimerHandle;

	UFUNCTION()
	void OnBuffDurationEnded();
};