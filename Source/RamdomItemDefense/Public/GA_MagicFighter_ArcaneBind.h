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
	UGA_MagicFighter_ArcaneBind(); // [수정] 생성자 선언 추가

protected:
	/** 어빌리티 활성화 시 (Event.Attack.Execute.Skill3 수신 시) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (Stun GE) 폭발 시 적용할 2초 스턴 GameplayEffect (블루프린트에서 GE_Stun_2s 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|GAS")
	TSubclassOf<UGameplayEffect> StunEffectClass;
	
};
