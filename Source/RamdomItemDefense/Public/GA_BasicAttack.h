#pragma once

#include "RamdomItemDefense.h" // ������Ʈ ��� ����
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
	/** �����Ƽ Ȱ��ȭ �� (Event.Attack.Execute.Basic ���� ��) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (�������Ʈ���� ����) �⺻ ���� �������� ������ GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** (�������Ʈ���� ����) ������ ��꿡 ����� ���ݷ� ��� (��: 1.0 = ���ݷ� 100%) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	float DamageCoefficient;

	/** (�������Ʈ���� ����) SetByCaller�� ������ ���� ������ �� ����� �±� */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	FGameplayTag DamageDataTag;
};
