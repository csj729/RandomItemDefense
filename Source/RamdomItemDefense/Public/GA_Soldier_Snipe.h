// Public/GA_Soldier_Snipe.h

#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Soldier_Snipe.generated.h"

class UGameplayEffect;
class AProjectileBase;

/**
 * 솔져 스킬 3: 저격
 * 단일 대상에게 큰 데미지를 주고 3초 스턴을 적용합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_Snipe : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Soldier_Snipe();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (BP 설정) 적용할 3초 스턴 GameplayEffect (예: GE_Stun_3s) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** (BP 설정) 스폰할 시각 효과용 투사체 (예: BP_Projectile_SniperBullet) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;

	/** (BP 설정) 투사체 속도 (타이머 계산용) (예: 10000) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	float VisualProjectileSpeed;

private:
	FTimerHandle ImpactTimerHandle;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	/** 투사체 도착 시 실제 데미지/효과를 적용하는 함수 */
	UFUNCTION()
	void OnImpact();
};