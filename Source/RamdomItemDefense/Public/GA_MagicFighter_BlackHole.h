// SSource/RamdomItemDefense/Public/GA_MagicFighter_BlackHole.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "GA_UltimateSkill.h" // 부모 클래스
#include "GA_MagicFighter_BlackHole.generated.h"

// 전방 선언
class UGameplayEffect;
class UParticleSystem;
class AMonsterAIController;

/**
 * @brief GA_UltimateSkill의 자식 클래스로, 실제 블랙홀(데미지, 스턴, 끌어당기기, AI 리셋) 로직을 C++로 구현합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_BlackHole : public UGA_UltimateSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_BlackHole();

protected:
	// --- (필수) 부모의 함수를 오버라이드 ---

	/** 어빌리티 활성화 시 (C++ 부모의 ActivateAbility 호출 후 실행됨) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 어빌리티 종료 시 (C++ 부모의 EndAbility 호출 전 실행됨) */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 적용된 상태 GE 핸들 (나중에 제거하기 위해 저장) */
	FActiveGameplayEffectHandle UltimateStateEffectHandle;

	/** (에디터에서 설정) 캐릭터에게 'State.Player.IsUsingUltimate' 태그를 부여할 GE (GE_State_IsUsingUltimate) */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> UltimateStateEffectClass;

	/** 몽타주가 '성공적으로' 종료되었을 때 호출될 콜백 함수 */
	UFUNCTION()
	void OnMontageFinished();

	/** 몽타주가 '중단되거나 취소'되었을 때 호출될 콜백 함수 */
	UFUNCTION()
	void OnMontageCancelled();


	// --- (블랙홀 로직용) 블루프린트 설정 변수 ---

	/** (BP 설정) 광역 데미지 및 스턴을 적용할 반경 (예: 1500) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config")
	float DamageRadius;

	/** (BP 설정) 5초 스턴 GE (예: GE_Stun_5s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** (BP 설정) 데미지 태그 (예: Skill.Damage.Value) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|GAS")
	FGameplayTag DamageDataTag;

	/** (BP 설정) 몬스터를 끌어당기는 시간 (예: 3.0초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|Pull")
	float PullDuration;

	/** (BP 설정) 몬스터를 끌어당기는 속도 (VInterpTo 속도) (예: 10.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|Pull")
	float PullInterpSpeed;

	/** (★★★) (BP 설정) 끌어당기기 및 데미지 틱 간격 (예: 0.1초 = 초당 10회) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|Pull")
	float PullTickInterval;

	/** (BP 설정) 블랙홀 위치(스포너)에 스폰할 파티클 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|FX")
	TObjectPtr<UParticleSystem> BlackHoleEffect;

	/** (BP 설정) 시전자(캐릭터)에게 스폰할 파티클 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlackHole Config|FX")
	TObjectPtr<UParticleSystem> CasterEffect;


	// --- (블랙홀 로직용) 내부 변수 ---
private:
	/** 끌어당기기 타이머 핸들 (Tick) */
	FTimerHandle PullTimerHandle;
	/** 끌어당기기 종료 타이머 핸들 (Duration) */
	FTimerHandle PullDurationTimerHandle;

	/** (★★★) 틱마다 적용할 데미지 GE 스펙 (미리 계산해서 저장) */
	FGameplayEffectSpecHandle DamageSpecHandle;

	/** (★★★) 시전자의 ASC 캐시 (PullTick에서 사용) */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	/** (★★★) 시전자의 위치 캐시 (PullTick에서 사용) */
	FVector CasterLocation;

	/** 블랙홀 중심 위치 (저장용) */
	FVector BlackHoleLocation;

	/** 몽타주가 종료되었는지 여부 */
	bool bMontageFinished;
	/** 끌어당기기가 종료되었는지 여부 */
	bool bPullFinished;

	/** 실제 끌어당기기 로직 (매 틱 실행) */
	void PullTick();
	/** 끌어당기기 종료 및 AI 리셋 */
	void StopPullingAndResetAI();

	/** 몽타주와 끌어당기기가 모두 끝났는지 확인하고 어빌리티를 종료합니다. */
	void CheckEndAbility();
};