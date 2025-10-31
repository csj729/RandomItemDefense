// Source/RamdomItemDefense/Public/GA_MagicFighter_DarkPulse.h

#pragma once

#include "RamdomItemDefense.h" 
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_DarkPulse.generated.h"

// --- [�ڵ� �߰�] ---
class UParticleSystem;
class USoundBase;
class ADarkPulseProjectile; // �ð� ȿ���� ����ü Ŭ������ �ٽ� ���� ����
// --- [�ڵ� �߰� ��] ---

class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_DarkPulse : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_DarkPulse();

protected:
	/** �����Ƽ�� ������ Ȱ��ȭ�� �� (Gameplay Event ����) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [�ڵ� ����] ---
	// ProjectileClass�� �ٽ� �߰��մϴ� (�ð� ȿ����)
	/** (�������Ʈ���� ����) ������ �ð� ȿ���� ����ü Ŭ���� (BP_DarkPulseProjectile) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<ADarkPulseProjectile> ProjectileClass;

	/** (�������Ʈ���� ����) �ð������� ���ư��� ����ü �ӵ� (��: 1500) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float VisualProjectileSpeed;

	/** (�������Ʈ���� ����) ���� �ݰ� (��: 300) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ExplosionRadius;

	/** (�������Ʈ���� ����) ���� �� ������ ���� ����Ʈ */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	/** (�������Ʈ���� ����) ���� ���� */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<USoundBase> ExplosionSound;
	// --- [�ڵ� ���� ��] ---

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

private:
	/** ���� Ÿ�̸� �ڵ� */
	FTimerHandle ImpactTimerHandle;

	/** ������ �Ͼ ��ġ (Ÿ���� �ʱ� ��ġ) */
	UPROPERTY()
	FVector TargetImpactLocation;

	/**
	 * @brief Ÿ�̸� ���� �� ���� ���� �� �������� �����ϴ� �Լ�
	 */
	UFUNCTION()
	void Explode();
};