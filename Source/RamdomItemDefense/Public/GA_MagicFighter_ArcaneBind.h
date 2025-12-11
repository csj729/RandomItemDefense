#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_ArcaneBind.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_ArcaneBind : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_ArcaneBind();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	/** 스턴 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;
};