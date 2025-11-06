#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Archer_CripplingShot.generated.h"

// --- [코드 수정] ---
class AProjectileBase; // (이름 변경)
// --- [코드 수정 끝] ---
class AActor;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_CripplingShot : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_CripplingShot();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (BP 설정) 4초 스턴 GE (예: GE_Stun_4s) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** (BP 설정) 방어력 30 감소 GE (예: GE_Archer_ArmorShred) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> ArmorShredEffectClass;

	// --- [코드 수정] ---
	/** (BP 설정) 스폰할 시각 효과용 투사체 (예: BP_Projectile_ArcherHeavyArrow) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass; // (이름 변경)
	// --- [코드 수정 끝] ---

	/** (BP 설정) 투사체 속도 (타이머 계산용) (예: 2000) */
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