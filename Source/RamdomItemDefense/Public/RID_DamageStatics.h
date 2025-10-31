// Source/RamdomItemDefense/Public/RID_DamageStatics.h (����)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RID_DamageStatics.generated.h"

class UAbilitySystemComponent;
class UMyAttributeSet;
class AActor;

/**
 * @brief ġ��Ÿ ������ �ؽ�Ʈ�� ǥ���ϱ� ���� ��������Ʈ
 * @param TargetActor �������� ���� ��� ����
 * @param CritDamageAmount ���� ����� ġ��Ÿ ������ ��
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCritDamageOccurredDelegate, AActor*, TargetActor, float, CritDamageAmount);

/**
 * @brief ������ ��� ���� ��ƿ��Ƽ �Լ��� ��Ƶ� ����ƽ Ŭ����
 */
UCLASS()
class RAMDOMITEMDEFENSE_API URID_DamageStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief (���� ����) �⺻ �������� ������� ġ��Ÿ�� ���/����/����մϴ�.
	 * @param BaseDamage ġ��Ÿ�� ����Ǳ� ���� �⺻ ������ (���)
	 * @param SourceASC �������� AbilitySystemComponent
	 * @param TargetActor �������� ���� ��� ���� (ġ��Ÿ �ؽ�Ʈ ǥ�ÿ�)
	 * @param bIsSkillAttack �� ������ ��ų���� ����
	 * @return ġ��Ÿ�� ����� ���� ������ (���)
	 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static float ApplyCritDamage(float BaseDamage, UAbilitySystemComponent* SourceASC, AActor* TargetActor, bool bIsSkillAttack);

	// --- [�ڵ� �߰�] ---
	/**
	 * @brief (���� ��ų��) ġ��Ÿ�� �߻��ߴ��� ������ �����մϴ�.
	 * @param SourceASC �������� ASC
	 * @param bIsSkillAttack ��ų ���� ���� (��ų Ȯ�� ����)
	 * @return ġ��Ÿ �߻� �� true
	 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static bool CheckForCrit(UAbilitySystemComponent* SourceASC, bool bIsSkillAttack);

	/**
	 * @brief (���� ��ų��) ���� ���� ���� ġ��Ÿ ������ ����մϴ�.
	 * @param SourceASC �������� ASC
	 * @return ġ��Ÿ ���� (��: 2.3)
	 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static float GetCritMultiplier(UAbilitySystemComponent* SourceASC);
	// --- [�ڵ� �߰� ��] ---

	/** ġ��Ÿ �߻� �� ��۵Ǵ� ���� ��������Ʈ */
	static FOnCritDamageOccurredDelegate OnCritDamageOccurred;

private:
	static const UMyAttributeSet* GetAttributeSetFromASC(UAbilitySystemComponent* SourceASC);
};