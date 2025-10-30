// Public/GA_BaseSkill.h (�� ����)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_BaseSkill.generated.h"

class UGameplayEffect;

/**
 * @brief ��� Ȯ�� �ߵ��� ��ų�� �θ� Ŭ����.
 * ���� �Ӽ�(�ߵ� Ȯ��, ������ GE ��)�� �����մϴ�.
 */
UCLASS(Abstract) // �� Ŭ������ ���� BP�� ������ ���ϵ��� '�߻�' Ŭ������ ����
class RAMDOMITEMDEFENSE_API UGA_BaseSkill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BaseSkill();

	/**
	 * @brief �� ��ų�� �⺻ �ߵ� Ȯ�� (0.0 ~ 1.0)
	 * GA_AttackSelecter�� �� ���� �о�ͼ� Ȯ�� ��꿡 ����մϴ�.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config")
	float BaseActivationChance;

	/**
	 * @brief �� ��ų�� ������ ������ GE
	 * (SetByCaller�� ����� ���� �ְ�, �ƴ� ���� �����Ƿ� Base������ TSubclassOf�� ����)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/**
	 * @brief ������ GE�� SetByCaller�� ���� ������ �� ����� �±�
	 * (�ʿ� ���� ��ų�� ����θ� ��)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Config|GAS")
	FGameplayTag DamageByCallerTag;
};