#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h" // FGameplayTag ����� ���� �߰�
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_MeteorStrike.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_MeteorStrike : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_MeteorStrike();

	/** GA_AttackSelecter�� �� �����Ƽ�� ȣ���� �� ����˴ϴ�. */
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	/** ���׿��� �������� �� �ɸ��� �ð� (��) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float FallDuration = 0.1f;

	/** ���� �ݰ� (��ȹ: 500) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ExplosionRadius = 500.0f;

	/** Ÿ�� ��ġ���� �󸶳� ������ ��ȯ���� (Z��) - ����Ʈ ������ */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float SpawnHeight = 2000.0f;

	/** (Stun GE) ���� �� ������ 2�� ���� GameplayEffect (�������Ʈ���� GE_Stun_2s ����) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/** �ϴÿ��� �������� ���׿� ��ƼŬ (Velocity�� ������ ��ƼŬ) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> FallingEffect;

	/** �ٴڿ��� �����ϴ� ��ƼŬ */
	UPROPERTY(EditDefaultsOnly, Category = "Config|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

private:
	/** FallDuration ���� Explode �Լ��� ȣ���ϱ� ���� Ÿ�̸� �ڵ� */
	FTimerHandle ImpactTimerHandle;

	/** ���� ���� (Ÿ���� �ʱ� ��ġ) */
	UPROPERTY()
	FVector ImpactLocation;

	/** Ÿ�̸Ӱ� ����Ǿ��� �� ���� ������ �����ϴ� �Լ� */
	UFUNCTION()
	void Explode();
};