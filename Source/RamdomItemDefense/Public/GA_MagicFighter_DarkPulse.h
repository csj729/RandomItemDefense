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
	/** 어빌리티가 실제로 활성화될 때 (Gameplay Event 포함) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (블루프린트에서 설정) 스폰할 투사체 클래스 (BP_DarkPulseProjectile) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<ADarkPulseProjectile> ProjectileClass;

	/** (블루프린트에서 설정) 플레이어 전방 몇 cm 앞에서 발사할지 (예: 100) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float SpawnDistanceForward;

	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> SlowEffectClass;
};

