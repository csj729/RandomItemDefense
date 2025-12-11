#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Archer_CripplingShot.generated.h"

class AProjectileBase;
class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Archer_CripplingShot : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Archer_CripplingShot();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	/** 스턴 효과 (4초) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** 방어력 감소 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> ArmorShredEffectClass;

	/** 발사할 투사체 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Projectile")
	float VisualProjectileSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Projectile")
	FName ProjectileSpawnSocketName;

private:
	// --- [ Internal State ] ---
	FTimerHandle ImpactTimerHandle;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UFUNCTION()
	void OnImpact();
};