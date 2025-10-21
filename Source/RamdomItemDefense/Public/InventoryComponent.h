#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "ItemTypes.h"
#include "InventoryComponent.generated.h"

class UAbilitySystemComponent;
class UDataTable;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RAMDOMITEMDEFENSE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	/**
	 * @brief �� ������Ʈ�� �ʱ�ȭ�ϰ� �������� ASC�� �����մϴ�.
	 * Character�� PossessedBy, OnRep_PlayerState���� ȣ��Ǿ�� �մϴ�.
	 * @param InASC ������(ĳ����)�� AbilitySystemComponent
	 */
	void Initialize(UAbilitySystemComponent* InASC);

	/**
	 * @brief (���� ����) �κ��丮�� �������� �߰��ϰ� GAS ȿ���� �����մϴ�.
	 * @param ItemID �߰��� �������� ID (������ ���̺��� Row Name)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(FName ItemID);

	/**
	 * @brief (���� ����) �κ��丮���� �������� �����ϰ� GAS ȿ���� �����մϴ�.
	 * @param ItemID ������ �������� ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItem(FName ItemID);

	/**
	 * @brief (���� ����) ������ ������ ���̺��� ������ �������� �ϳ� �̾� �߰��մϴ�.
	 * (���� ���� �� '������ �̱�'��)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddRandomItem();

	/** UI ���ε��� ��������Ʈ */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryUpdatedDelegate OnInventoryUpdated;

protected:
	/** ������ ������ ����ִ� ������ ���̺� (�������Ʈ���� ����) */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	/**
	 * @brief (�ʼ�) 1�ܰ迡�� ������ 'GE_Item_Stats_SBC' �������Ʈ�� ���⿡ �����ؾ� �մϴ�.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|GAS")
	TSubclassOf<UGameplayEffect> GenericItemStatEffect;

private:
	/**
	 * @brief ItemID�� ������ ���̺��� FItemData�� ã���ϴ�.
	 */
	const FItemData* FindItemDataByID(FName ItemID) const;

	/**
	 * @brief EItemStatType�� SetByCaller�� FGameplayTag�� ��ȯ�մϴ�.
	 */
	FGameplayTag GetTagFromStatType(EItemStatType StatType) const;

	/** ������ ASC�� ������ ������ */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** ���� ���� ���� ������ ID ��� */
	UPROPERTY(VisibleInstanceOnly, Category = "Inventory")
	TArray<FName> InventoryItems;

	/** ����� ���� ȿ��(GE) �ڵ� �� (���ſ�) */
	UPROPERTY()
	TMap<FName, FActiveGameplayEffectHandle> AppliedStatEffectHandles;

	/** �ο��� ���� �ɷ�(GA) �ڵ� �� (���ſ�) */
	UPROPERTY()
	TMap<FName, FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};