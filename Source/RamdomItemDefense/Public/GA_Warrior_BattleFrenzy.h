// Public/GA_Warrior_BattleFrenzy.h

#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Warrior_BattleFrenzy.generated.h"

class UGameplayEffect;
class UParticleSystem;
class UAbilityTask_WaitGameplayEffectRemoved;

/**
 * 전사 스킬 2: 전투의 함성
 * 공격 시 확률적으로 5초간 공격력에 비례한 공격 속도 버프를 얻습니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Warrior_BattleFrenzy : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Warrior_BattleFrenzy();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** (BP 설정) 적용할 공격 속도 버프 GameplayEffect (SetByCaller, 5초 지속시간) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> AttackSpeedBuffEffectClass;

	/** (BP 설정) 버프 지속 시간 (예: 5.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float BuffDuration;

	/** (BP 설정) 공격 속도 변환 계수 (예: 0.1 = 공격력 100당 공속 10% 증가) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float AttackSpeedCoefficient;

	/** (BP 설정) 공속 버프 SetByCaller 태그 (예: State.Player.Warrior.AttackSpeed) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag AttackSpeedBuffTag;

	/** * (BP 설정) 이 버프 GE가 적용될 때 부여할 태그 (예: State.Player.Warrior.AttackSpeed.Active)
	 * 이 태그는 GE 블루프린트의 'Granted Tags'에도 정확히 일치하게 추가되어야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|GAS")
	FGameplayTag BuffIsActiveTag;

	/** (BP 설정) 버프가 활성화되는 동안 캐릭터에 Attach할 이펙트 (예: P_BattleFrenzy_Buff) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffEffect;

	/** (BP 설정) 이펙트를 Attach할 소켓 이름 (예: "WeaponSocket" 또는 "Root") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|FX")
	FName AttachSocketName;

private:
	/** 적용된 버프 GE 핸들 (제거 추적용) */
	FActiveGameplayEffectHandle ActiveBuffEffectHandle;

	// --- [ ★★★ 추가 ★★★ ] ---
	/** 스폰된 버프 이펙트 컴포넌트 (제거용) */
	UPROPERTY()
	UParticleSystemComponent* ActiveBuffFXComponent;

	/** 버프 GE가 제거될 때 호출될 함수 */
	UFUNCTION()
	void OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo);

	bool bIsBuffOn = false;
};