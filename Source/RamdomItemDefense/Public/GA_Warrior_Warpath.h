// Public/GA_Warrior_Warpath.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_Warrior_Warpath.generated.h"

class UGameplayEffect;
class UParticleSystem;
class UParticleSystemComponent;
class UNiagaraSystem;
class UNiagaraComponent;

/**
 * 전사 궁극기: 전쟁의 길
 * 맵의 모든 적에게 큰 피해를 주고, 20초간 모든 스킬 발동 확률 15% 버프를 얻습니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Warrior_Warpath : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_Warrior_Warpath();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** (BP 설정) 캐릭터에게 'State.Player.IsUsingUltimate' 태그를 부여할 GE (GE_State_IsUsingUltimate) */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> UltimateStateEffectClass;

	/** (BP 설정) 스킬 발동 확률 15% 증가, 20초 지속 버프 GE (예: GE_Warrior_Warpath_Buff) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> UltimateBuffEffectClass;

	/** (BP 설정) 데미지 태그 (예: Skill.Damage.Value) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Damage")
	FGameplayTag DamageDataTag;

	/** (BP 설정) 맵 전체로 간주할 광역 데미지 반경 (예: 10000) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Damage")
	float DamageRadius;

	/** (BP 설정) 시전자(캐릭터)에게 스폰할 이펙트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> CasterEffect;

	/** (BP 설정) 버프가 지속되는 동안 캐릭터에 Attach할 '지속' 이펙트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffEffect;

	/** (BP 설정) 지속 이펙트를 Attach할 소켓 이름 (예: "Root") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FName BuffEffectAttachSocketName;

	/** 스폰된 지속 버프 이펙트 컴포넌트 (제거용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag BuffIsActiveTag;

private:
	/** 몽타주 종료 콜백 */
	UFUNCTION()
	void OnMontageFinished();

	/** 몽타주 취소 콜백 */
	UFUNCTION()
	void OnMontageCancelled();

	/** 버프 GE가 제거될 때 호출될 함수 */
	UFUNCTION()
	void OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo);

	/** 적용된 상태 GE 핸들 (제거용) */
	FActiveGameplayEffectHandle UltimateStateEffectHandle;

	/** 적용된 버프 GE 핸들 (제거용) */
	FActiveGameplayEffectHandle UltimateBuffEffectHandle;
};