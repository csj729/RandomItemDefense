// csj729/randomitemdefense/RandomItemDefense-78a128504f0127dc02646504d4a1e1c677a0e811/Source/RamdomItemDefense/Public/InventoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffect.h" 
#include "Abilities/GameplayAbility.h" 
#include "ItemTypes.h" 
#include "InventoryComponent.generated.h"

class UAbilitySystemComponent;
class UDataTable;

// --- [�ڵ� �߰�] ---
/** �������� ���� GE �ڵ��� ItemID�� �Բ� �����ϱ� ���� ����ü */
USTRUCT()
struct FActiveItemStatEffect
{
	GENERATED_BODY()

	/** �� �ڵ��� ���� �������� ID */
	UPROPERTY()
	FName ItemID;

	/** ����� ���� GameplayEffect�� ���� �ڵ� */
	UPROPERTY()
	FActiveGameplayEffectHandle Handle;
};

/** �������� ���� �����Ƽ �ڵ��� ItemID�� �Բ� �����ϱ� ���� ����ü */
USTRUCT()
struct FActiveItemAbility
{
	GENERATED_BODY()

	/** �� �ڵ��� ���� �������� ID */
	UPROPERTY()
	FName ItemID;

	/** �ο��� GameplayAbility�� ���� �ڵ� */
	UPROPERTY()
	FGameplayAbilitySpecHandle Handle;
};
// --- [�ڵ� �߰� ��] ---


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RAMDOMITEMDEFENSE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	void Initialize(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddRandomItem();

	/**
	 * @brief (���� ���� / UI ȣ���) ��� ������ ID�� ������� ������ ������ �õ��մϴ�.
	 * @param ResultItemID ���� ��� �������� ID
	 * @return ���� ���� ����
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Combine")
	bool CombineItemByResultID(FName ResultItemID);

	/**
	 * @brief (Ŭ��/����) Ư�� ���չ��� �ʿ��� ��Ḧ ��� ������ �ִ��� Ȯ���մϴ�.
	 * @param RecipeID ���չ� ID (������ ���̺��� Row Name)
	 * @return ���� ���� ����
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Combine")
	bool CanCombine(FName RecipeID) const;

	/** @brief ���� �κ��丮 ������ ����� ��ȯ�մϴ�. (UI���� ���) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FName>& GetInventoryItems() const { return InventoryItems; }

	/** @brief ������ ID�� ������ �����͸� �����ɴϴ�. (UI���� ���) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FItemData GetItemData(FName ItemID, bool& bSuccess) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UDataTable* GetRecipeDataTable() const { return RecipeDataTable; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryUpdatedDelegate OnInventoryUpdated;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	/** ���չ� ������ ����ִ� ������ ���̺� (�������Ʈ���� ����) */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UDataTable> RecipeDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|GAS")
	TSubclassOf<UGameplayEffect> GenericItemStatEffect;

private:
	FGameplayTag GetTagFromStatType(EItemStatType StatType) const;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleInstanceOnly, Category = "Inventory")
	TArray<FName> InventoryItems;

	// --- [�ڵ� ����] TMap -> TArray<struct> ---
	/** (��) TArray: �������� '�� �ν��Ͻ�'���� ���� GE �ڵ��� �����մϴ�. */
	UPROPERTY()
	TArray<FActiveItemStatEffect> ActiveStatEffects;

	/** (��) TArray: �������� '�� �ν��Ͻ�'���� �ο��� GA �ڵ��� �����մϴ�. */
	UPROPERTY()
	TArray<FActiveItemAbility> ActiveGrantedAbilities;
	// --- [�ڵ� ���� ��] ---
};