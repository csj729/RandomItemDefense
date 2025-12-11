#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_MagicFighter_BlackHole.generated.h"

class UGameplayEffect;
class UParticleSystem;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_BlackHole : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_BlackHole();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- [ Configuration ] ---
	/** 블랙홀 반경 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config")
	float DamageRadius;

	/** 끌어당기는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|Pull")
	float PullDuration;

	/** 끌어당기는 속도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|Pull")
	float PullInterpSpeed;

	/** 데미지/스턴 틱 간격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|Pull")
	float PullTickInterval;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "BlackHole Config|GAS")
	TSubclassOf<UGameplayEffect> UltimateStateEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|GAS")
	FGameplayTag DamageDataTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|FX")
	TObjectPtr<UParticleSystem> BlackHoleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|FX")
	TObjectPtr<UParticleSystem> CasterEffect;

private:
	// --- [ Internal State ] ---
	FTimerHandle PullTimerHandle;
	FTimerHandle PullDurationTimerHandle;
	FGameplayEffectSpecHandle DamageSpecHandle;
	FActiveGameplayEffectHandle UltimateStateEffectHandle;

	UPROPERTY() TObjectPtr<UAbilitySystemComponent> SourceASC;
	FVector CasterLocation;
	FVector BlackHoleLocation;

	bool bMontageFinished;
	bool bPullFinished;

	// --- [ Logic ] ---
	void PullTick();
	void StopPullingAndResetAI();
	void CheckEndAbility();

	UFUNCTION() void OnMontageFinished();
	UFUNCTION() void OnMontageCancelled();
};