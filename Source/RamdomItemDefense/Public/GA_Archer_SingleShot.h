#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Archer_SingleShot.generated.h"

// --- [코드 수정] ---
class AProjectileBase; // (이름 변경)
// --- [코드 수정 끝] ---
class AActor;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_SingleShot : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_SingleShot();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [코드 수정] ---
	/** (BP 설정) 스폰할 시각 효과용 투사체 (예: BP_Projectile_ArcherArrow) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass; // (이름 변경)
	// --- [코드 수정 끝] ---

	/** (BP 설정) 투사체 속도 (타이머 계산용) (예: 2500) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	float VisualProjectileSpeed;

private:
	FTimerHandle ImpactTimerHandle;

	/** 데미지를 적용할 타겟 (타이머 만료 시 사용) */
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	/** 투사체 도착 시 실제 데미지를 적용하는 함수 */
	UFUNCTION()
	void OnImpact();
};