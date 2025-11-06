#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_Archer_UltimateFocus.generated.h" // 이름 변경

class UParticleSystem;
class UParticleSystemComponent;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_UltimateFocus : public UGA_UltimateSkill // 이름 변경
{
	GENERATED_BODY()

public:
	UGA_Archer_UltimateFocus(); // 이름 변경

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 궁극기가 종료될 때 (버프 FX 제거용) */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** (BP 설정) 20초간 지속되는 실제 스탯 버프 GE (공속+100%, 치피+100% 등) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> FocusBuffEffectClass;

	/** (BP 설정) 궁극기 '사용 시' 재생할 이펙트 (1회성) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ActivationEffect;

	/** (BP 설정) 궁극기 '지속 중' 캐릭터에게 붙어있을 이펙트 (20초간) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffEffect;

private:
	/** 20초 뒤 EndAbility를 호출하기 위한 타이머 */
	FTimerHandle BuffDurationTimerHandle;

	/** 20초간 지속되는 스탯 버프 GE 핸들 (제거용) */
	FActiveGameplayEffectHandle FocusBuffEffectHandle;

	/** 20초간 지속되는 버프 파티클 컴포넌트 (제거용) */
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> BuffEffectComponent;

	/** 20초가 만료되었을 때 호출될 함수 */
	UFUNCTION()
	void OnBuffDurationEnded();
};