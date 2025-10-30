// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GA_MagicFighter_ArcaneBind.generated.h"

/**
 * 
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_MagicFighter_ArcaneBind : public UGA_BaseSkill
{
	GENERATED_BODY()

public:
	UGA_MagicFighter_ArcaneBind(); // [����] ������ ���� �߰�

protected:
	/** �����Ƽ Ȱ��ȭ �� (Event.Attack.Execute.Skill3 ���� ��) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (Stun GE) ���� �� ������ 2�� ���� GameplayEffect (�������Ʈ���� GE_Stun_2s ����) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;
	
};
