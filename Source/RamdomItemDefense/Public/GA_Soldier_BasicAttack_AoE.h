#pragma once

#include "CoreMinimal.h"
#include "GA_BasicAttack.h"
#include "GA_Soldier_BasicAttack_AoE.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_BasicAttack_AoE : public UGA_BasicAttack
{
	GENERATED_BODY()

public:
	UGA_Soldier_BasicAttack_AoE();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 광역 기본 공격의 범위 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AoERadius;
};