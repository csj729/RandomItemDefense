#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Archer_SingleShot.generated.h"

class AProjectileBase;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_SingleShot : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_SingleShot();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	/** 발사할 투사체 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;

	/** 투사체 속도 (타이머 계산용) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	float VisualProjectileSpeed;

	/** 발사 소켓 이름 (예: MuzzleSocket) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Projectile")
	FName ProjectileSpawnSocketName;

private:
	// --- [ Internal State ] ---
	FTimerHandle ImpactTimerHandle;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	/** 투사체 도착 시 데미지 적용 */
	UFUNCTION()
	void OnImpact();
};