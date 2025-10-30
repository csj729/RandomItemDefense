#pragma once

#include "RamdomItemDefense.h" 
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_DarkPulse.generated.h"

class ADarkPulseProjectile;
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

	/** (�������Ʈ���� ����) ������ ����ü Ŭ���� (BP_DarkPulseProjectile) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<ADarkPulseProjectile> ProjectileClass;

	/** (�������Ʈ���� ����) �÷��̾� ���� �� cm �տ��� �߻����� (��: 100) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float SpawnDistanceForward;

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;
};

