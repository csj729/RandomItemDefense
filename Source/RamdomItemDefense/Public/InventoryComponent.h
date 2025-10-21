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
	 * @brief 이 컴포넌트를 초기화하고 소유자의 ASC에 연결합니다.
	 * Character의 PossessedBy, OnRep_PlayerState에서 호출되어야 합니다.
	 * @param InASC 소유자(캐릭터)의 AbilitySystemComponent
	 */
	void Initialize(UAbilitySystemComponent* InASC);

	/**
	 * @brief (서버 전용) 인벤토리에 아이템을 추가하고 GAS 효과를 적용합니다.
	 * @param ItemID 추가할 아이템의 ID (데이터 테이블의 Row Name)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(FName ItemID);

	/**
	 * @brief (서버 전용) 인벤토리에서 아이템을 제거하고 GAS 효과를 해제합니다.
	 * @param ItemID 제거할 아이템의 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItem(FName ItemID);

	/**
	 * @brief (서버 전용) 아이템 데이터 테이블에서 무작위 아이템을 하나 뽑아 추가합니다.
	 * (라운드 시작 시 '아이템 뽑기'용)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddRandomItem();

	/** UI 바인딩용 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryUpdatedDelegate OnInventoryUpdated;

protected:
	/** 아이템 정보가 들어있는 데이터 테이블 (블루프린트에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UDataTable> ItemDataTable;

	/**
	 * @brief (필수) 1단계에서 생성한 'GE_Item_Stats_SBC' 블루프린트를 여기에 설정해야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|GAS")
	TSubclassOf<UGameplayEffect> GenericItemStatEffect;

private:
	/**
	 * @brief ItemID로 데이터 테이블에서 FItemData를 찾습니다.
	 */
	const FItemData* FindItemDataByID(FName ItemID) const;

	/**
	 * @brief EItemStatType을 SetByCaller용 FGameplayTag로 변환합니다.
	 */
	FGameplayTag GetTagFromStatType(EItemStatType StatType) const;

	/** 소유자 ASC의 약참조 포인터 */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** 현재 보유 중인 아이템 ID 목록 */
	UPROPERTY(VisibleInstanceOnly, Category = "Inventory")
	TArray<FName> InventoryItems;

	/** 적용된 스탯 효과(GE) 핸들 맵 (제거용) */
	UPROPERTY()
	TMap<FName, FActiveGameplayEffectHandle> AppliedStatEffectHandles;

	/** 부여된 고유 능력(GA) 핸들 맵 (제거용) */
	UPROPERTY()
	TMap<FName, FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};