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
	/** �����Ƽ Ȱ��ȭ �� (Event.Attack.Perform ���� ��) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * @brief (�������Ʈ ������) Ȯ���� ���� ������ ���� ���� �±׸� �����մϴ�.
	 * @param TriggerEventData ���� ��� ���� ���� ���޿�
	 * @return ���õ� ���� ���� �±� (��: Event.Attack.Execute.Basic)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Attack Selection")
	FGameplayTag SelectAttackType(const FGameplayEventData& TriggerEventData);

	/**
	 * @brief (�������Ʈ ������) ������ ���� �±׷� Gameplay Event�� �����մϴ�.
	 * @param TargetActor ���� ���
	 * @param ExecuteTag ������ �̺�Ʈ �±�
	 */
	UFUNCTION(BlueprintCallable, Category = "Attack Selection")
	void SendExecuteAttackEvent(AActor* TargetActor, FGameplayTag ExecuteTag);

};