// Public/GA_Soldier_BasicAttack_AoE.h

#pragma once

#include "CoreMinimal.h"
#include "GA_BasicAttack.h"
#include "GA_Soldier_BasicAttack_AoE.generated.h"

/**
 * 솔져 궁극기 전용 광역 기본 공격.
 * (GE_Soldier_AllOutWar_Buff에 의해 부여됨)
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_BasicAttack_AoE : public UGA_BasicAttack
{
	GENERATED_BODY()

public:
	UGA_Soldier_BasicAttack_AoE();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (BP 설정) 기본 공격의 광역 반경 (기획: 200) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AoERadius;
};