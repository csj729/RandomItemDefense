// Source/RamdomItemDefense/Public/InventoryComponent.h (수정)

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

// 3. 인벤토리 전용 커스텀 로그 매크로를 정의합니다.
#define ENABLE_INVENTORY_DEBUG 0

// 3. 인벤토리 전용 로그 카테고리를 선언합니다.
DECLARE_LOG_CATEGORY_EXTERN(LogRID_Inventory, Log, All);

// 4. 인벤토리 전용 커스텀 로그 매크로를 정의합니다.
// (마스터 스위치 && 개별 스위치 모두 1일 때만 코드를 생성)
#if ENABLE_RID_DEBUG && ENABLE_INVENTORY_DEBUG

#include "Engine/Engine.h" // GEngine 사용을 위해 포함

// [ ★★★ 수정: Verbosity 파라미터 제거 ★★★ ]
#define LOG_INVENTORY(Color, Format, ...) \
	{ \
		if (GEngine) \
		{ \
			FString Msg = FString::Printf(Format, ##__VA_ARGS__); \
			GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, FString::Printf(TEXT("[Inv] %s"), *Msg)); \
			/* [ ★★★ 수정: Verbosity를 'Log'로 고정 ★★★ ] */ \
			UE_LOG(LogRID_Inventory, Log, TEXT("%s"), *Msg); \
		} \
	}
#else

// [ ★★★ 수정: Verbosity 파라미터 제거 ★★★ ]
#define LOG_INVENTORY(Color, Format, ...) (void)0

#endif

/** 아이템의 스탯 GE 핸들을 ItemID와 함께 추적하기 위한 구조체 */
USTRUCT()
struct FActiveItemStatEffect
{
	GENERATED_BODY()

	/** 이 핸들이 속한 아이템의 ID */
	UPROPERTY()
	FName ItemID;

	/** 적용된 스탯 GameplayEffect의 실제 핸들 */
	UPROPERTY()
	FActiveGameplayEffectHandle Handle;
};

/** 아이템의 고유 어빌리티 핸들을 ItemID와 함께 추적하기 위한 구조체 */
USTRUCT()
struct FActiveItemAbility
{
	GENERATED_BODY()

	/** 이 핸들이 속한 아이템의 ID */
	UPROPERTY()
	FName ItemID;

	/** 부여된 GameplayAbility의 실제 핸들 */
	UPROPERTY()
	FGameplayAbilitySpecHandle Handle;
};


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

	/** @brief 아이템 ID로 아이템 데이터를 가져옵니다. (UI에서 사용) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FItemData GetItemData(FName ItemID, bool& bSuccess) const;

	// --- [ ★★★ 코드 수정 ★★★ ] ---
	/**
	 * @brief 데이터 테이블에서 '흔함' 등급 아이템 ID를 '모두' 찾아 배열로 반환합니다.
	 * @return 모든 흔함 아이템 ID 배열
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FName> GetAllCommonItemIDs() const;
	// --- [ ★★★ 코드 수정 끝 ★★★ ] ---

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UDataTable* GetRecipeDataTable() const { return RecipeDataTable; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInventoryUpdatedDelegate OnInventoryUpdated;

	// [추가] 변수 복제를 위한 함수 오버라이드
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	UPROPERTY(VisibleInstanceOnly, Category = "Inventory", ReplicatedUsing = OnRep_InventoryItems)
	TArray<FName> InventoryItems;

	UFUNCTION()
	void OnRep_InventoryItems();

	/** (신) TArray: 아이템의 '각 인스턴스'별로 스탯 GE 핸들을 추적합니다. */
	UPROPERTY()
	TArray<FActiveItemStatEffect> ActiveStatEffects;

	/** (신) TArray: 아이템의 '각 인스턴스'별로 부여된 GA 핸들을 추적합니다. */
	UPROPERTY()
	TArray<FActiveItemAbility> ActiveGrantedAbilities;
};