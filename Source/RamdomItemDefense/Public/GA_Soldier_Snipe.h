#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_Soldier_Snipe.generated.h"

class AProjectileBase;
class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_Snipe : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_Soldier_Snipe();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Configuration ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

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