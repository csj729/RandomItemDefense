// Source/RamdomItemDefense/Public/GA_MagicFighter_DarkPulse.h

#pragma once

#include "RamdomItemDefense.h" 
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_DarkPulse.generated.h"

class UParticleSystem;
class USoundBase;
class AProjectileBase;
class UGameplayEffect; // [ ★★★ 헤더 추가 ★★★ ]

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_DarkPulse : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_DarkPulse();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (블루프린트에서 설정) 스폰할 시각 효과용 투사체 클래스 (BP_Projectile_DarkPulse) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<AProjectileBase> ProjectileClass;

	/** (블루프린트에서 설정) 시각적으로 날아가는 투사체 속도 (예: 1500) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float VisualProjectileSpeed;

	/** (블루프린트에서 설정) 폭발 반경 (예: 300) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ExplosionRadius;

	/** (블루프린트에서 설정) 적중 시 스폰할 폭발 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	/** (블루프린트에서 설정) 폭발 사운드 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<USoundBase> ExplosionSound;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	/** (BP 설정) 이펙트를 스폰할 소켓 이름 (예: "MuzzleSocket") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	FName ProjectileSpawnSocketName;
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

private:
	/** 폭발 타이머 핸들 */
	FTimerHandle ImpactTimerHandle;

	/** 폭발이 일어날 위치 (타겟의 초기 위치) */
	UPROPERTY()
	FVector TargetImpactLocation;

	/** (타이머가 참조할 타겟) */
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	/**
	 * @brief 타이머 만료 시 실제 폭발 및 데미지를 적용하는 함수
	 */
	UFUNCTION()
	void Explode();
};