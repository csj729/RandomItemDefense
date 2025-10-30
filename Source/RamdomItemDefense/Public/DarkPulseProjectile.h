#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "DarkPulseProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
// --- [코드 수정] ---
class UParticleSystemComponent; // Niagara -> Particle System
class UParticleSystem;         // Niagara -> Particle System
// --- [코드 수정 끝] ---
class USoundBase;
class UAbilitySystemComponent;
class UGameplayEffect;
class AMonsterBaseCharacter;


UCLASS()
class RAMDOMITEMDEFENSE_API ADarkPulseProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADarkPulseProjectile();

	void Initialize(AActor* InTargetActor, UAbilitySystemComponent* InOwnerASC, TSubclassOf<UGameplayEffect> InDamageEffectClass);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	void Explode();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// --- [코드 수정] ---
	/** (블루프린트에서 설정) 투사체가 날아가는 동안 보일 이펙트 (꼬리 등) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> TrailEffect; // Niagara -> Particle System
	// --- [코드 수정 끝] ---

	// --- [코드 수정] ---
	/** (블루프린트에서 설정) 적중 시 스폰할 폭발 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> ExplosionEffect; // Niagara -> Particle System
	// --- [코드 수정 끝] ---

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ArrivalTolerance;

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	bool bHasExploded;
};

