#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GameplayTagContainer.h"
#include "GA_BasicAttack.generated.h"

class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_BasicAttack : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_BasicAttack();

protected:
	// --- [ Overrides ] ---
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Helper Functions ] ---
	void ApplySlowEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float SlowPercent, float Duration);
	void ApplyStunEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Duration);
	void ApplyArmorShredEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float ArmorAmount, float Duration);

	// --- [ Configuration : Effects ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Effects")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Effects")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Effects")
	TSubclassOf<UGameplayEffect> ArmorReductionEffectClass;

	// --- [ Configuration : Tags ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag DamageDataTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag SlowAmountTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag ArmorShredAmountTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Tags")
	FGameplayTag DurationTag;
};