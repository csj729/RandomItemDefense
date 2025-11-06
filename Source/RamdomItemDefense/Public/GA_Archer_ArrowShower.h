#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Archer_ArrowShower.generated.h"

class UMyAttributeSet;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_ArrowShower : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_ArrowShower();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** (BP에서 설정) 장판이 지속되는 시간 (예: 3.0초) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float Duration;

	/** (BP에서 설정) 장판이 몬스터를 감지하는 주기 (예: 0.2초) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float TickInterval;

	/** (BP에서 설정) 화살비가 내리꽂힐 반경 (예: 400) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float Radius;

	/** (BP에서 설정) 화살비 시각 효과 (Particle) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ArrowShowerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

private:
	/** 장판 효과가 지속되는 시간(Duration)을 재는 타이머 */
	FTimerHandle EffectDurationTimerHandle;
	/** 장판 효과를 주기적으로(TickInterval) 적용하는 타이머 */
	FTimerHandle EffectTickTimerHandle;

	/** 장판의 중심 위치 */
	FVector TargetLocation;

	/** EffectTickTimerHandle이 주기적으로 호출할 함수 */
	UFUNCTION()
	void OnEffectTick();

	/** EffectDurationTimerHandle이 3초 뒤 호출할 함수 */
	UFUNCTION()
	void OnEffectEnd();
};