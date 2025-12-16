#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Soldier_DronePassive.generated.h"

class APawn;
class UGameplayEffect;
class UAbilitySystemComponent;
class ARamdomItemDefenseCharacter;
class AProjectileBase;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_DronePassive : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Soldier_DronePassive();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// --- [ Configuration : Drone ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TSubclassOf<APawn> DroneClassToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TSubclassOf<UGameplayEffect> DroneStatInitEffect;

	/** 드론에게 부여할 어빌리티 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TArray<TSubclassOf<UGameplayAbility>> DroneAbilities;

	// --- [ Configuration : Passive Attack ] ---
	/** 공격/스캔 주기 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Attack")
	float AttackRate;

	/** 스캔 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Attack")
	float ScanRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Attack")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Attack")
	float VisualProjectileSpeed;

	/** 드론 공격 데미지 (기본값) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Attack")
	float DamageBase;

	/** 드론 공격 계수 (주인 스탯 비례) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Attack")
	float DamageCoefficient;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Attack")
	FName MuzzleSocketName;

private:
	// --- [ Internal State ] ---
	UPROPERTY() TObjectPtr<APawn> SpawnedDrone;
	UPROPERTY() TWeakObjectPtr<UAbilitySystemComponent> OwnerASC_Cache;
	UPROPERTY() TWeakObjectPtr<UAbilitySystemComponent> DroneASC_Cache;

	FActiveGameplayEffectHandle DroneStatEffectHandle;
	TArray<FDelegateHandle> AttributeDelegateHandles;
	FTimerHandle ScanTimerHandle;

	// --- [ Logic ] ---
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	void OnOwnerStatChanged(const struct FOnAttributeChangeData& Data);
	UFUNCTION() void UpdateDroneStats();
};