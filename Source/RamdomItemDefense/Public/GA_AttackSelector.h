#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_AttackSelector.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_AttackSelector : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_AttackSelector();

protected:
	/** 어빌리티 활성화 시 (Event.Attack.Perform 수신 시) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * @brief (블루프린트 구현용) 확률에 따라 실행할 공격 유형 태그를 결정합니다.
	 * @param TriggerEventData 공격 대상 등의 정보 전달용
	 * @return 선택된 공격 실행 태그 (예: Event.Attack.Execute.Basic)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Attack Selection")
	FGameplayTag SelectAttackType(const FGameplayEventData& TriggerEventData);

	/**
	 * @brief (블루프린트 구현용) 결정된 공격 태그로 Gameplay Event를 전송합니다.
	 * @param TargetActor 공격 대상
	 * @param ExecuteTag 전송할 이벤트 태그
	 */
	UFUNCTION(BlueprintCallable, Category = "Attack Selection")
	void SendExecuteAttackEvent(AActor* TargetActor, FGameplayTag ExecuteTag);

};