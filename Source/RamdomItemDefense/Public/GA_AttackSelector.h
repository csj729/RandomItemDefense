#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_AttackSelector.generated.h"

class UAbilitySystemComponent;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_AttackSelector : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_AttackSelector();

protected:
	// --- [ Overrides ] ---
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// --- [ Blueprint Logic ] ---
	/** 확률에 따라 실행할 공격 태그 결정 (BP 구현) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Attack Selection")
	FGameplayTag SelectAttackType(const FGameplayEventData& TriggerEventData, UAbilitySystemComponent* OwnerASC);

	// --- [ Helper Functions ] ---
	/** 결정된 공격 태그로 이벤트 전송 */
	UFUNCTION(BlueprintCallable, Category = "Attack Selection")
	void SendExecuteAttackEvent(AActor* TargetActor, FGameplayTag ExecuteTag);
};