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

	// --- [�ڵ� ����] ---
	/** @brief ������ ID�� ������ �����͸� �����ɴϴ�. (UI���� ���) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FItemData GetItemData(FName ItemID, bool& bSuccess) const;
	// ------------------

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

	UPROPERTY()
	TMap<FName, FActiveGameplayEffectHandle> AppliedStatEffectHandles;

	UPROPERTY()
	TMap<FName, FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};