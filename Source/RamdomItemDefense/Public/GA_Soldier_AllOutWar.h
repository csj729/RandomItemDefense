// Public/GA_Soldier_AllOutWar.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_Soldier_AllOutWar.generated.h"

class UGameplayEffect;
class UParticleSystem;
class UParticleSystemComponent;

/**
 * 솔져 궁극기: 총력전
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_AllOutWar : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_Soldier_AllOutWar();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> UltimateStateEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> AllOutWarBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag AttackSpeedBuffTag;

	/** (BP 설정) 버프가 지속되는 동안 캐릭터에 Attach할 '지속' 이펙트 (캐스케이드) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffEffect;

	/** (BP 설정) 지속 이펙트를 Attach할 소켓 이름 (예: "Root") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FName BuffEffectAttachSocketName;

	// --- [ ★★★ 추가 ★★★ ] ---
	/** (BP 설정) 지속 이펙트의 스케일 (크기) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FVector BuffEffectScale;
	// --- [ ★★★ 추가 끝 ★★★ ] ---

private:
	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo);

	FActiveGameplayEffectHandle UltimateStateEffectHandle;
	FActiveGameplayEffectHandle UltimateBuffEffectHandle;

	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> BuffEffectComponent;
};