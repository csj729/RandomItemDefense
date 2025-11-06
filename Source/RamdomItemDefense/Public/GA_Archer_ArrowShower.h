#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Archer_ArrowShower.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_ArrowShower : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_ArrowShower();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** (BP에서 설정) 화살이 떨어지기 시작할 때까지의 딜레이 (예: 0.5초) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float InitialDelay;

	/** (BP에서 설정) 화살비가 내리꽂힐 반경 (예: 400) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float Radius;

	/** (BP에서 설정) 3초간 지속되는 데미지(DoT) 및 슬로우 GE */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> ArrowShowerEffectClass; // (GE_Archer_ArrowShower_DoTAndSlow)

	/** (BP에서 설정) 화살비 시각 효과 (Particle) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ArrowShowerEffect;

private:
	FTimerHandle EffectTimerHandle;
	FVector TargetLocation;

	/** 딜레이 후 실제 효과를 적용하는 함수 */
	UFUNCTION()
	void ApplyEffect();
};