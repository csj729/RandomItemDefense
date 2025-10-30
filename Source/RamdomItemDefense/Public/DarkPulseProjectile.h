#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "DarkPulseProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
// --- [�ڵ� ����] ---
class UParticleSystemComponent; // Niagara -> Particle System
class UParticleSystem;         // Niagara -> Particle System
// --- [�ڵ� ���� ��] ---
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

	// --- [�ڵ� ����] ---
	/** (�������Ʈ���� ����) ����ü�� ���ư��� ���� ���� ����Ʈ (���� ��) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> TrailEffect; // Niagara -> Particle System
	// --- [�ڵ� ���� ��] ---

	// --- [�ڵ� ����] ---
	/** (�������Ʈ���� ����) ���� �� ������ ���� ����Ʈ */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> ExplosionEffect; // Niagara -> Particle System
	// --- [�ڵ� ���� ��] ---

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

