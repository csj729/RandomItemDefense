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
	 * @brief (서버 전용 / UI 호출용) 결과 아이템 ID를 기반으로 아이템 조합을 시도합니다.
	 * @param ResultItemID 조합 결과 아이템의 ID
	 * @return 조합 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Combine")
	bool CombineItemByResultID(FName ResultItemID);

	/**
	 * @brief (클라/서버) 특정 조합법에 필요한 재료를 모두 가지고 있는지 확인합니다.
	 * @param RecipeID 조합법 ID (데이터 테이블의 Row Name)
	 * @return 조합 가능 여부
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Combine")
	bool CanCombine(FName RecipeID) const;

	/** @brief 현재 인벤토리 아이템 목록을 반환합니다. (UI에서 사용) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FName>& GetInventoryItems() const { return InventoryItems; }

	// --- [코드 수정] ---
	/** @brief 아이템 ID로 아이템 데이터를 가져옵니다. (UI에서 사용) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FItemData GetItemData(FName ItemID, bool& bSuccess) const;
	// ------------------

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryUpdatedDelegate OnInventoryUpdated;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	/** 조합법 정보가 들어있는 데이터 테이블 (블루프린트에서 설정) */
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