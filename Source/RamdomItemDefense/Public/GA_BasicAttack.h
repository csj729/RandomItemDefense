#pragma once

#include "RamdomItemDefense.h" // 프로젝트 헤더 먼저
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_BasicAttack.generated.h"

class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_BasicAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BasicAttack();

protected:
	/** 어빌리티 활성화 시 (Event.Attack.Execute.Basic 수신 시) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (블루프린트에서 설정) 기본 공격 데미지를 적용할 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** (블루프린트에서 설정) 데미지 계산에 사용할 공격력 계수 (예: 1.0 = 공격력 100%) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float DamageCoefficient;

	/** (블루프린트에서 설정) SetByCaller로 데미지 값을 전달할 때 사용할 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	FGameplayTag DamageDataTag;
};
