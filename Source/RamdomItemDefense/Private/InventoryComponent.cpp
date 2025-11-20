// Source/RamdomItemDefense/Private/InventoryComponent.cpp (수정)

#include "InventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h" // MakeEffectContext
#include "Engine/DataTable.h"
#include "Engine/Engine.h" // GEngine 디버그 메시지
#include "GameplayTagsManager.h" // GameplayTag 관련
#include "RamdomItemDefense.h" // RID_LOG 매크로용 (이제 LOG_INVENTORY가 대체)
#include "Net/UnrealNetwork.h"

// --- [ ★★★ 로그 카테고리 정의 ★★★ ] ---
// .h 파일에서 선언한 로그 카테고리를 여기서 정의(구현)합니다.
DEFINE_LOG_CATEGORY(LogRID_Inventory);
// --- [ ★★★ 로그 카테고리 정의 끝 ★★★ ] ---


UInventoryComponent::UInventoryComponent()
{
	// 이 컴포넌트는 서버에만 존재하므로 Tick은 필요 없습니다.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// [추가] 복제할 변수 등록
void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// InventoryItems 배열을 복제 대상으로 등록
	DOREPLIFETIME(UInventoryComponent, InventoryItems);
}

void UInventoryComponent::OnRep_InventoryItems()
{
	// 클라이언트 UI에게 "데이터 바꼈으니 다시 그려!"라고 알림
	UE_LOG(LogRamdomItemDefense, Warning, TEXT("OnRep_InventoryItems Called! Item Count: %d"), InventoryItems.Num());
	OnInventoryUpdated.Broadcast();

	// 로그 확인용
	// LOG_INVENTORY(FColor::Green, TEXT("Client Inventory Updated! Count: %d"), InventoryItems.Num());
}

void UInventoryComponent::Initialize(UAbilitySystemComponent* InASC)
{
	AbilitySystemComponent = InASC;
}

/**
 * @brief 데이터 테이블에서 아이템 정보를 찾아 복사본을 반환합니다.
 * @param ItemID 가져올 아이템 ID
 * @param bSuccess 아이템 데이터를 성공적으로 찾았는지 여부 (출력)
 * @return 아이템 데이터 (복사본), 찾지 못하면 기본값
 */
FItemData UInventoryComponent::GetItemData(FName ItemID, bool& bSuccess) const
{
	bSuccess = false; // 기본적으로 실패로 설정
	if (!ItemDataTable)
	{
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		// RID_LOG(FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set!"));
		LOG_INVENTORY(FColor::Red, TEXT("ItemDataTable is NOT set!"));
		// ---------------------------------
		return FItemData(); // 빈 구조체 반환
	}

	// 데이터 테이블에서 ItemID(Row Name)에 해당하는 FItemData 행을 찾습니다.
	FItemData* Row = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("InventoryComponent::GetItemData"));
	if (Row)
	{
		bSuccess = true; // 찾았으면 성공으로 설정
		return *Row;     // 찾은 데이터의 복사본(*)을 반환
	}

	// 찾지 못했으면 빈 구조체를 반환합니다.
	return FItemData();
}

/**
 * @brief EItemStatType 열거형을 SetByCaller용 FGameplayTag로 변환합니다.
 */
FGameplayTag UInventoryComponent::GetTagFromStatType(EItemStatType StatType) const
{
	// .ini 파일에 등록한 태그 이름들을 사용합니다.
	switch (StatType)
	{
	case EItemStatType::AttackDamage:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.AttackDamage"));
	case EItemStatType::AttackSpeed:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.AttackSpeed"));
	case EItemStatType::CritChance:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.CritChance"));
	case EItemStatType::CritDamage:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.CritDamage"));
	case EItemStatType::ArmorReduction:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.ArmorReduction"));
	case EItemStatType::MoveSpeedReduction:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.MoveSpeedReduction"));
	case EItemStatType::SkillActivationChance:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.SkillActivationChance"));
	case EItemStatType::StunChance: // MyAttributeSet에 추가되었다고 가정
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.StunChance"));
	default:
		return FGameplayTag::EmptyTag; // 유효하지 않은 타입이면 빈 태그 반환
	}
}

/** (서버 전용) 아이템 추가 */
void UInventoryComponent::AddItem(FName ItemID)
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;

	bool bFound = false;
	const FItemData ItemData = GetItemData(ItemID, bFound);
	if (!bFound) return;

	// 1. 스탯 효과(GE) 적용 (SetByCaller)
	if (ItemData.BaseStats.Num() > 0 && GenericItemStatEffect)
	{
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GenericItemStatEffect, 1.0f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			for (const FItemStatData& StatData : ItemData.BaseStats)
			{
				FGameplayTag StatTag = GetTagFromStatType(StatData.StatType);
				if (StatTag.IsValid())
				{
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(StatTag, StatData.Value);
				}
			}
			FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			// --- [디버그 로그 추가] ---
			if (ActiveHandle.IsValid())
			{
				// --- [코드 수정] TMap::Add -> TArray::Add ---
				ActiveStatEffects.Add({ ItemID, ActiveHandle });
				// -----------------------------------------
				// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
				LOG_INVENTORY(FColor::Cyan, TEXT("AddItem: Stored GE Handle for %s"), *ItemID.ToString());
				// ---------------------------------
			}
			else
			{
				// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
				LOG_INVENTORY(FColor::Red, TEXT("AddItem: Applied GE Handle for %s is INVALID!"), *ItemID.ToString());
				// ---------------------------------
			}
		}
	}

	// 2. 고유 능력(GA) 부여 (변경 없음)
	if (ItemData.GrantedAbility)
	{
		FGameplayAbilitySpec AbilitySpec(ItemData.GrantedAbility);
		FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);

		// --- [코드 수정] TMap::Add -> TArray::Add ---
		if (AbilityHandle.IsValid())
		{
			ActiveGrantedAbilities.Add({ ItemID, AbilityHandle });
			// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
			LOG_INVENTORY(FColor::Cyan, TEXT("AddItem: Stored GA Handle for %s"), *ItemID.ToString());
			// ---------------------------------
		}
		else
		{
			// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
			LOG_INVENTORY(FColor::Red, TEXT("AddItem: Applied GA Handle for %s is INVALID!"), *ItemID.ToString());
			// ---------------------------------
		}
		// --- [코드 수정 끝] ---
	}

	InventoryItems.Add(ItemID);
	OnRep_InventoryItems();
	// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
	LOG_INVENTORY(FColor::Green, TEXT("Item Added (SBC): %s"), *ItemID.ToString());
	// ---------------------------------
}

/** (서버 전용) 아이템 제거 */
void UInventoryComponent::RemoveItem(FName ItemID)
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;

	// --- [코드 수정] TMap::Contains -> TArray::IndexOfByPredicate ---

	// 1. 스탯 효과(GE) 제거
	// ItemID와 일치하는 '첫 번째' 스탯 효과 항목을 배열에서 찾습니다.
	int32 StatEffectIndex = ActiveStatEffects.IndexOfByPredicate(
		[ItemID](const FActiveItemStatEffect& Effect) { return Effect.ItemID == ItemID; }
	);

	// 항목을 찾았다면
	if (StatEffectIndex != INDEX_NONE)
	{
		// 핸들을 가져옵니다.
		FActiveGameplayEffectHandle HandleToRemove = ActiveStatEffects[StatEffectIndex].Handle;
		if (HandleToRemove.IsValid())
		{
			// ASC에서 GE를 제거합니다.
			bool bRemoved = AbilitySystemComponent->RemoveActiveGameplayEffect(HandleToRemove);
			if (bRemoved)
			{
				// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
				LOG_INVENTORY(FColor::Orange, TEXT("RemoveItem: Removed GE Handle for %s (Success)"), *ItemID.ToString());
				// ---------------------------------
			}
			else
			{
				// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
				LOG_INVENTORY(FColor::Red, TEXT("RemoveItem: Failed to remove GE Handle for %s"), *ItemID.ToString());
				// ---------------------------------
			}
		}
		else
		{
			// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
			LOG_INVENTORY(FColor::Red, TEXT("RemoveItem: Found INVALID GE Handle in array for %s"), *ItemID.ToString());
			// ---------------------------------
		}

		// '반드시' 추적 배열에서 이 항목을 제거합니다. (다음 RemoveItem 호출 시 그 다음 항목을 찾도록)
		ActiveStatEffects.RemoveAt(StatEffectIndex);
	}
	else
	{
		// 스탯이 없는 아이템일 수 있으므로 경고만 출력
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Yellow, TEXT("RemoveItem: No GE Handle found in array for %s"), *ItemID.ToString());
		// ---------------------------------
	}
	// --- [코드 수정 끝] ---


	// 2. 고유 능력(GA) 제거
	// --- [코드 수정] TMap::Contains -> TArray::IndexOfByPredicate ---

	// ItemID와 일치하는 '첫 번째' 어빌리티 항목을 배열에서 찾습니다.
	int32 AbilityIndex = ActiveGrantedAbilities.IndexOfByPredicate(
		[ItemID](const FActiveItemAbility& Ability) { return Ability.ItemID == ItemID; }
	);

	// 항목을 찾았다면
	if (AbilityIndex != INDEX_NONE)
	{
		// 핸들을 가져옵니다.
		FGameplayAbilitySpecHandle HandleToRemove = ActiveGrantedAbilities[AbilityIndex].Handle;
		if (HandleToRemove.IsValid())
		{
			// ASC에서 어빌리티를 제거합니다.
			AbilitySystemComponent->ClearAbility(HandleToRemove);
			// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
			LOG_INVENTORY(FColor::Orange, TEXT("RemoveItem: Removed GA Handle for %s (Success)"), *ItemID.ToString());
			// ---------------------------------
		}
		else
		{
			// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
			LOG_INVENTORY(FColor::Red, TEXT("RemoveItem: Found INVALID GA Handle in array for %s"), *ItemID.ToString());
			// ---------------------------------
		}

		// '반드시' 추적 배열에서 이 항목을 제거합니다.
		ActiveGrantedAbilities.RemoveAt(AbilityIndex);
	}
	else
	{
		// 어빌리티가 없는 아이템일 수 있으므로 경고만 출력
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Yellow, TEXT("RemoveItem: No GA Handle found in array for %s"), *ItemID.ToString());
		// ---------------------------------
	}
	// --- [코드 수정 끝] ---


	// 3. 실제 인벤토리 배열에서 제거 (가장 마지막에 수행)
	bool bRemovedFromArray = InventoryItems.RemoveSingle(ItemID) > 0;
	if (!bRemovedFromArray) // 아이템이 배열에 없는데 제거 시도된 경우
	{
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Red, TEXT("RemoveItem: ItemID %s was not found in InventoryItems array!"), *ItemID.ToString());
		// ---------------------------------
		return; // 아이템이 없었으므로 UI 업데이트 불필
	}
	// --------------------------------------------------------

	OnRep_InventoryItems(); // UI 갱신 알림
	// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
	LOG_INVENTORY(FColor::Yellow, TEXT("Item Removed: %s"), *ItemID.ToString());
	// ---------------------------------
}

/**
 * @brief (서버 전용) 아이템 데이터 테이블에서 무작위 아이템을 하나 뽑아 추가합니다.
 * ('아이템 뽑기' 선택 시 MyPlayerState에서 호출)
 */
void UInventoryComponent::AddRandomItem()
{
	// ItemDataTable 유효성 검사
	if (!ItemDataTable)
	{
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Red, TEXT("ItemDataTable is NOT set! Cannot add random item."));
		// ---------------------------------
		return;
	}

	// 데이터 테이블의 모든 행 이름(아이템 ID)을 가져옵니다.
	TArray<FName> AllItemIDs = ItemDataTable->GetRowNames();
	if (AllItemIDs.Num() == 0) return; // 아이템이 없으면 종료

	// --- [코드 수정] ---
	// 흔함 등급 아이템 ID만 저장할 새 배열을 만듭니다.
	TArray<FName> CommonItemIDs;

	// 모든 아이템 ID를 순회합니다.
	for (const FName& ItemID : AllItemIDs)
	{
		bool bSuccess = false;
		// 해당 아이템 ID의 데이터를 가져옵니다.
		FItemData ItemData = GetItemData(ItemID, bSuccess);
		// 데이터를 성공적으로 가져왔고, 등급이 'Common'인지 확인합니다.
		if (bSuccess && ItemData.Grade == EItemGrade::Common)
		{
			// 조건을 만족하면 CommonItemIDs 배열에 추가합니다.
			CommonItemIDs.Add(ItemID);
		}
	}

	// 흔함 등급 아이템이 하나라도 있는지 확인합니다.
	if (CommonItemIDs.Num() > 0)
	{
		// CommonItemIDs 배열에서 무작위 인덱스를 선택합니다.
		int32 RandomIndex = FMath::RandRange(0, CommonItemIDs.Num() - 1);
		FName RandomCommonItemID = CommonItemIDs[RandomIndex];

		// 선택된 흔함 등급 아이템 ID로 AddItem 함수를 호출합니다.
		AddItem(RandomCommonItemID);
	}
	else
	{
		// 데이터 테이블에 흔함 등급 아이템이 하나도 없을 경우 로그 출력
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Yellow, TEXT("No Common grade items found in ItemDataTable!"));
		// ---------------------------------
	}
	// ------------------
}

// --- [ ★★★ 코드 추가 ★★★ ] ---
/**
 * @brief 데이터 테이블에서 '흔함' 등급 아이템 ID를 '모두' 찾아 배열로 반환합니다.
 */
TArray<FName> UInventoryComponent::GetAllCommonItemIDs() const
{
	TArray<FName> CommonItemIDs;
	if (!ItemDataTable)
	{
		return CommonItemIDs;
	}

	// 1. 모든 아이템 ID를 순회합니다.
	TArray<FName> AllItemIDs = ItemDataTable->GetRowNames();
	for (const FName& ItemID : AllItemIDs)
	{
		bool bSuccess = false;
		// 해당 아이템 ID의 데이터를 가져옵니다.
		FItemData ItemData = GetItemData(ItemID, bSuccess);
		// 데이터를 성공적으로 가져왔고, 등급이 'Common'인지 확인합니다.
		if (bSuccess && ItemData.Grade == EItemGrade::Common)
		{
			// 조건을 만족하면 CommonItemIDs 배열에 추가합니다.
			CommonItemIDs.Add(ItemID);
		}
	}

	// 2. '흔함' 등급 아이템 전체 목록을 반환합니다.
	return CommonItemIDs;
}
// --- [ ★★★ 코드 추가 끝 ★★★ ] ---


/**
 * @brief (서버 전용 / UI 호출용) 결과 아이템 ID를 기반으로 아이템 조합을 시도합니다.
 * @param ResultItemID 조합 결과 아이템의 ID (DT_Recipes의 Row Name 또는 ResultItemID 필드)
 * @return 조합 성공 여부
 */
bool UInventoryComponent::CombineItemByResultID(FName ResultItemID)
{
	// 서버에서만 실행되도록 합니다.
	if (!GetOwner()->HasAuthority())
	{
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Red, TEXT("CombineItemByResultID called on Client!"));
		// ---------------------------------
		return false;
	}
	// RecipeDataTable이 설정되어 있는지 확인합니다.
	if (!RecipeDataTable)
	{
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Red, TEXT("RecipeDataTable is NOT set!"));
		// ---------------------------------
		return false;
	}

	// 1. 결과 아이템 ID로 조합법 데이터를 찾습니다.
	// 먼저 Row Name(Name 컬럼)이 ResultItemID와 일치하는지 찾습니다.
	FRecipeData* RecipeData = RecipeDataTable->FindRow<FRecipeData>(ResultItemID, TEXT("InventoryComponent::CombineItemByResultID"));

	// Row Name으로 못 찾았다면, 모든 조합법을 순회하며 ResultItemID 필드가 일치하는 것을 찾습니다.
	// (데이터 테이블 설계에 따라 이 부분이 필요 없을 수도 있습니다)
	if (!RecipeData)
	{
		TArray<FName> RowNames = RecipeDataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FRecipeData* TempRecipe = RecipeDataTable->FindRow<FRecipeData>(RowName, "");
			if (TempRecipe && TempRecipe->ResultItemID == ResultItemID)
			{
				RecipeData = TempRecipe;
				break; // 찾았으면 루프 종료
			}
		}
	}

	// 최종적으로 조합법을 찾지 못했다면 실패를 반환합니다.
	if (!RecipeData)
	{
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Yellow, TEXT("No recipe found for ResultItemID: %s"), *ResultItemID.ToString());
		// ---------------------------------
		return false;
	}

	// 2. 이 조합법으로 조합이 가능한지(재료가 충분한지) CanCombine 함수로 확인합니다.
	// (CanCombine 함수는 내부적으로 RecipeID를 사용하므로, 찾은 조합법의 실제 Row Name을 넘겨줘야 할 수도 있습니다.
	// 여기서는 임시로 ResultItemID를 RecipeID처럼 사용합니다. 필요시 수정 필요)
	if (!CanCombine(ResultItemID)) // TODO: RecipeData->GetRowName() 같은 함수가 있다면 그것을 사용
	{
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Yellow, TEXT("Cannot combine %s: Ingredients missing."), *ResultItemID.ToString());
		// ---------------------------------
		return false;
	}

	// 3. 재료 아이템들을 인벤토리에서 소모(제거)합니다.
	// 안전하게 제거하기 위해, 제거 중 오류가 발생하는지 추적합니다.
	bool bFailedToRemoveIngredient = false;
	TArray<FName> TempInventoryForRemovalCheck = InventoryItems; // 현재 인벤토리 복사 (안전 체크용)
	for (const FName& IngredientID : RecipeData->Ingredients)
	{
		// 복사본에 해당 재료가 있는지 먼저 확인 (CanCombine 통과 후 변동 가능성 대비)
		if (TempInventoryForRemovalCheck.Contains(IngredientID))
		{
			RemoveItem(IngredientID); // 실제 인벤토리에서 아이템 제거 및 효과 해제
			TempInventoryForRemovalCheck.RemoveSingle(IngredientID); // 복사본에서도 제거
		}
		else
		{
			// 이 경우는 CanCombine() 로직이 잘못되었거나, 그 사이에 아이템이 사라진 경우입니다.
			bFailedToRemoveIngredient = true;
			// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
			LOG_INVENTORY(FColor::Red, TEXT("CRITICAL ERROR in CombineItem: Ingredient %s vanished AFTER CanCombine passed!"), *IngredientID.ToString());
			// ---------------------------------
			break; // 재료 소모 중단
		}
	}

	// 4. 모든 재료가 성공적으로 소모되었다면, 결과 아이템을 인벤토리에 추가합니다.
	if (!bFailedToRemoveIngredient)
	{
		AddItem(RecipeData->ResultItemID); // 결과 아이템 추가 및 효과 적용
		// --- [ ★★★ 로그 매크로 교체 ★★★ ] ---
		LOG_INVENTORY(FColor::Green, TEXT("Successfully combined: %s"), *RecipeData->ResultItemID.ToString());
		// ---------------------------------
		return true; // 조합 성공
	}

	// 재료 소모 중 오류가 발생했다면 실패를 반환합니다.
	// (고급: 여기서 이미 소모된 재료를 다시 되돌리는 롤백 로직을 추가할 수 있습니다)
	return false; // 조합 실패
}


/**
 * @brief (클라이언트/서버) 특정 조합법에 필요한 재료를 모두 가지고 있는지 확인합니다.
 * @param RecipeID 조합법 ID (데이터 테이블의 Row Name 또는 ResultItemID - 현재 로직 기준)
 * @return 조합 가능 여부
 */
bool UInventoryComponent::CanCombine(FName RecipeID) const
{
	// RecipeDataTable이 설정되어 있는지 확인합니다.
	if (!RecipeDataTable) return false;

	// 1. 조합법 데이터를 찾습니다. (CombineItemByResultID와 동일한 로직 사용)
	FRecipeData* RecipeData = RecipeDataTable->FindRow<FRecipeData>(RecipeID, TEXT("InventoryComponent::CanCombine"));
	if (!RecipeData)
	{
		TArray<FName> RowNames = RecipeDataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FRecipeData* TempRecipe = RecipeDataTable->FindRow<FRecipeData>(RowName, "");
			if (TempRecipe && TempRecipe->ResultItemID == RecipeID)
			{
				RecipeData = TempRecipe;
				break;
			}
		}
	}

	// 조합법이 없거나, 조합법에 필요한 재료 목록이 비어있으면 조합 불가
	if (!RecipeData || RecipeData->Ingredients.Num() == 0)
	{
		return false;
	}

	// 2. 현재 인벤토리 상태의 복사본을 만듭니다. (원본 인벤토리는 변경하지 않음)
	TArray<FName> TempInventory = InventoryItems;

	// 3. 조합법의 모든 재료(Ingredients)를 순회합니다.
	for (const FName& IngredientID : RecipeData->Ingredients)
	{
		// 복사본 인벤토리에 해당 재료가 있는지 확인합니다.
		if (TempInventory.Contains(IngredientID))
		{
			// 있다면, 복사본에서 해당 재료를 하나 제거합니다. (개수 체크 목적)
			TempInventory.RemoveSingle(IngredientID);
		}
		else
		{
			// 재료 중 하나라도 복사본에 없다면, 즉시 false(조합 불가)를 반환합니다.
			return false;
		}
	}

	// 4. 루프를 무사히 통과했다면 (모든 재료가 복사본에 존재했다면), true(조합 가능)를 반환합니다.
	return true;
}