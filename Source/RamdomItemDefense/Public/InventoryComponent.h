#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffect.h" 
#include "Abilities/GameplayAbility.h" 
#include "ItemTypes.h" 
#include "RamdomItemDefense.h" 
#include "InventoryComponent.generated.h"

class UAbilitySystemComponent;
class UDataTable;

/** 아이템의 스탯 GE 핸들 추적용 구조체 */
USTRUCT()
struct FActiveItemStatEffect
{
	GENERATED_BODY()

	UPROPERTY() FName ItemID;
	UPROPERTY() FActiveGameplayEffectHandle Handle;
};

/** 아이템의 어빌리티 핸들 추적용 구조체 */
USTRUCT()
struct FActiveItemAbility
{
	GENERATED_BODY()

	UPROPERTY() FName ItemID;
	UPROPERTY() FGameplayAbilitySpecHandle Handle;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RAMDOMITEMDEFENSE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- [ Initialization ] ---
	void Initialize(UAbilitySystemComponent* InASC);

	// --- [ Public API : Item Management ] ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItem(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddRandomItem();

	// --- [ Public API : Combination ] ---
	/** 결과 아이템 ID로 조합 시도 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Combine")
	bool CombineItemByResultID(FName ResultItemID);

	/** 조합 가능 여부 확인 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Combine")
	bool CanCombine(FName RecipeID) const;

	// --- [ Public Getters ] ---
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FName>& GetInventoryItems() const { return InventoryItems; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FItemData GetItemData(FName ItemID, bool& bSuccess) const;

	/** 모든 '흔함' 등급 아이템 ID 반환 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FName> GetAllCommonItemIDs() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UDataTable* GetRecipeDataTable() const { return RecipeDataTable; }

	// --- [ Delegates ] ---
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryUpdatedDelegate OnInventoryUpdated;

	// --- [ State ] ---
	UPROPERTY(VisibleInstanceOnly, Category = "Inventory", ReplicatedUsing = OnRep_InventoryItems)
	TArray<FName> InventoryItems;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UDataTable> RecipeDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|GAS")
	TSubclassOf<UGameplayEffect> GenericItemStatEffect;

private:
	FGameplayTag GetTagFromStatType(EItemStatType StatType) const;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UFUNCTION()
	void OnRep_InventoryItems();

	// --- [ Internal Tracking ] ---
	UPROPERTY()
	TArray<FActiveItemStatEffect> ActiveStatEffects;

	UPROPERTY()
	TArray<FActiveItemAbility> ActiveGrantedAbilities;
};