#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h"
#include "GA_Archer_UltimateFocus.generated.h"

class UParticleSystem;
class UParticleSystemComponent;
class USoundBase; // 사운드 전방 선언

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_UltimateFocus : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_UltimateFocus();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** (BP에서 설정) 궁극기 버프의 GameplayEffect Class (예: GE_Archer_Focus_Buff) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> UltimateBuffEffectClass;

	/** (BP에서 설정) 궁극기 버프의 지속시간 (초) (예: 20.0) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BuffDuration;

	// --- [ ★★★ 요청하신 3개의 파티클 변수 ★★★ ] ---
	/** (BP 설정) 궁극기 '사용 시' 재생할 이펙트 (1회성) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ActivationEffect;

	/** (BP 설정) 버프 발동 시 캐릭터 몸에 부착할 파티클 이펙트 (지속) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> BuffParticleSystem;

	/** (BP 설정) 버프 발동 시 캐릭터 머리 위에 띄울 추가 파티클 이펙트 (지속) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> HeadBuffParticleSystem;
	// --- [ ★★★ 코드 수정 끝 ★★★ ] ---

	/** (BP에서 설정) 버프 발동 시 재생할 사운드 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<USoundBase> BuffStartSound;

private:
	/** 버프 GE가 활성화되었을 때의 핸들 (제거용) */
	FActiveGameplayEffectHandle UltimateBuffEffectHandle;

	/** 버프 지속시간을 관리하는 타이머 핸들 */
	FTimerHandle BuffDurationTimerHandle;

	/** 몸(Body) 지속 파티클 컴포넌트 참조 (제거용) */
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> ActiveBuffParticleComponent;

	/** 머리(Head) 지속 파티클 컴포넌스 참조 (제거용) */
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> ActiveHeadBuffParticleComponent;

	/** 버프 지속 시간이 끝났을 때 호출될 함수 */
	UFUNCTION()
	void OnBuffDurationEnded();
};