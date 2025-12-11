#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_BaseSkill.generated.h"

class UGameplayEffect;
class UParticleSystem;

/**
 * @brief 모든 확률 발동형 스킬의 부모 클래스.
 * 공통 속성(발동 확률, 데미지 GE 등) 및 유틸리티 함수를 정의합니다.
 */
UCLASS(Abstract)
class RAMDOMITEMDEFENSE_API UGA_BaseSkill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BaseSkill();

	// --- [ Override Functions ] ---
	/** 어빌리티 활성화 시 공통 처리(MuzzleFlash 등) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	/** 기본 발동 확률 (0.0 ~ 1.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config")
	float BaseActivationChance;

	/** 데미지 적용용 GE 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** SetByCaller 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|GAS")
	FGameplayTag DamageByCallerTag;

	/** 기본 데미지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|Damage")
	float DamageBase;

	/** 공격력 계수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|Damage")
	float DamageCoefficient;

	/** 총구 화염 이펙트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|FX")
	TObjectPtr<UParticleSystem> MuzzleFlashEffect;

	/** 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|FX")
	FName MuzzleSocketName;

protected:
	// --- [ Helper Functions (Refactored) ] ---

	/** 캐릭터/드론의 소켓 위치와 타겟 회전을 계산 */
	void GetMuzzleTransform(AActor* AvatarActor, FVector& OutLocation, FRotator& OutRotation, AActor* TargetActor = nullptr);

	/** (범위 공격용) 데미지 Spec과 치명타 여부를 미리 계산 */
	FGameplayEffectSpecHandle MakeDamageEffectSpec(const FGameplayAbilityActorInfo* ActorInfo, float InBaseDmg, float InCoeff, float& OutFinalDamage, bool& OutDidCrit);

	/** (단일 타겟용) 타겟에게 즉시 데미지 계산 및 적용 */
	bool ApplyDamageToTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* Target, float InBaseDmg, float InCoeff);
};