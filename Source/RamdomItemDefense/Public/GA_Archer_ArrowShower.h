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
	// --- [ Configuration ] ---
	/** 장판 지속 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float Duration;

	/** 데미지/감지 틱 주기 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float TickInterval;

	/** 장판 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float Radius;

	/** 화살비 파티클 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ArrowShowerEffect;

	/** 틱마다 적용할 슬로우 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

private:
	// --- [ Internal State ] ---
	FTimerHandle EffectDurationTimerHandle;
	FTimerHandle EffectTickTimerHandle;

	FVector TargetLocation;

	UFUNCTION() void OnEffectTick();
	UFUNCTION() void OnEffectEnd();
};