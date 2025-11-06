#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_UltimateSkill.generated.h"

// 전방 선언
class UGameplayEffect;
class UAnimMontage;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_UltimateSkill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_UltimateSkill();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation") // <-- BlueprintReadOnly 추가!
	UAnimMontage* UltimateMontage;

protected:
	/** (★★★) 어빌리티가 활성화될 수 있는지 (스택이 찼는지) 확인합니다. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 어빌리티가 활성화되면(CanActivateAbility 통과 후) 호출됩니다. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 어빌리티가 어떤 이유로든 종료될 때(중요!) 호출됩니다. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 몽타주 애니메이션이 끝났을 때 호출될 함수 (파라미터 없음) */
	UFUNCTION()
	void OnMontageEnded();
};