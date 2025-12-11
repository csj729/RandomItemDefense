#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h" // 부모 클래스 변경
#include "GA_UltimateSkill.generated.h"

class UAnimMontage;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_UltimateSkill : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_UltimateSkill();

protected:
	// --- [ Config ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> UltimateMontage;

	// --- [ Overrides ] ---
	/** 궁극기 스택 확인 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 활성화 시 스택 소모 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 몽타주 종료 콜백 */
	UFUNCTION()
	void OnMontageEnded();
};