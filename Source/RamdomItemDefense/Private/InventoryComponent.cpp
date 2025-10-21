#include "InventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h" // MakeEffectContext
#include "Engine/DataTable.h"
#include "Engine/Engine.h" // GEngine 디버그 메시지
#include "GameplayTagsManager.h" // GameplayTag 관련

UInventoryComponent::UInventoryComponent()
{
	// 이 컴포넌트는 서버에만 존재하므로 Tick은 필요 없습니다.
	PrimaryComponentTick.bCanEverTick = false;
	// 복제 설정은 필요 없습니다.
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set!"));
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

/**
 * @brief (서버 전용) 인벤토리에 아이템을 추가하고 GAS 효과(SetByCaller) 및 어빌리티를 적용합니다.
 */
void UInventoryComponent::AddItem(FName ItemID)
{
	// ASC가 유효한지, 그리고 서버에서 실행되는지 확인합니다.
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent::AddItem called on Client!"));
		return;
	}

	// 데이터 테이블에서 아이템 정보를 가져옵니다.
	bool bFound = false;
	const FItemData ItemData = GetItemData(ItemID, bFound);
	if (!bFound)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("InventoryComponent: ItemID '%s' not found in ItemDataTable."), *ItemID.ToString()));
		return;
	}

	// 1. 스탯 효과(GE) 적용 (SetByCaller 방식)
	// 아이템 데이터에 BaseStats 정보가 있고, GenericItemStatEffect가 설정되어 있는지 확인합니다.
	if (ItemData.BaseStats.Num() > 0 && GenericItemStatEffect)
	{
		// GameplayEffect 컨텍스트를 생성합니다.
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this); // 이펙트 유발자를 InventoryComponent로 설정

		// GenericItemStatEffect를 기반으로 Spec 핸들을 생성합니다.
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GenericItemStatEffect, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 아이템 데이터의 모든 BaseStats를 순회하며 SetByCaller 값을 설정합니다.
			for (const FItemStatData& StatData : ItemData.BaseStats)
			{
				FGameplayTag StatTag = GetTagFromStatType(StatData.StatType); // 스탯 타입에 맞는 태그 가져오기
				if (StatTag.IsValid())
				{
					// Spec 데이터에 SetByCaller 값을 주입합니다 (예: "Item.Stat.AttackDamage" 태그에 10.0f)
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(StatTag, StatData.Value);
				}
			}

			// 모든 값이 설정된 Spec을 캐릭터 자신에게 적용합니다.
			FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			// 나중에 제거할 수 있도록 핸들을 아이템 ID와 함께 맵에 저장합니다.
			AppliedStatEffectHandles.Add(ItemID, ActiveHandle);
		}
	}

	// 2. 고유 능력(GA) 부여 (GameplayAbility)
	// 아이템 데이터에 GrantedAbility가 설정되어 있는지 확인합니다.
	if (ItemData.GrantedAbility)
	{
		// 어빌리티 Spec을 생성하고 캐릭터에게 부여합니다.
		FGameplayAbilitySpec AbilitySpec(ItemData.GrantedAbility);
		FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		// 나중에 제거할 수 있도록 핸들을 아이템 ID와 함께 맵에 저장합니다.
		GrantedAbilityHandles.Add(ItemID, AbilityHandle);
	}

	// 인벤토리 배열에 아이템 ID를 추가합니다.
	InventoryItems.Add(ItemID);
	// UI 갱신을 위해 델리게이트를 방송합니다.
	OnInventoryUpdated.Broadcast();

	// 로그 출력
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Item Added (SBC): %s"), *ItemID.ToString()));
}

/**
 * @brief (서버 전용) 인벤토리에서 아이템을 제거하고 GAS 효과 및 어빌리티를 해제합니다.
 */
void UInventoryComponent::RemoveItem(FName ItemID)
{
	// ASC 유효성, 서버 실행, 아이템 존재 여부를 확인합니다.
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;
	if (!InventoryItems.Contains(ItemID)) return;

	// 1. 스탯 효과(GE) 제거
	// 해당 아이템 ID로 저장된 GE 핸들이 있는지 확인합니다.
	if (AppliedStatEffectHandles.Contains(ItemID))
	{
		// 핸들을 사용하여 적용된 GameplayEffect를 제거합니다.
		AbilitySystemComponent->RemoveActiveGameplayEffect(AppliedStatEffectHandles[ItemID]);
		// 맵에서도 핸들 정보를 제거합니다.
		AppliedStatEffectHandles.Remove(ItemID);
	}

	// 2. 고유 능력(GA) 제거
	// 해당 아이템 ID로 저장된 GA 핸들이 있는지 확인합니다.
	if (GrantedAbilityHandles.Contains(ItemID))
	{
		// 핸들을 사용하여 부여된 GameplayAbility를 제거합니다.
		AbilitySystemComponent->ClearAbility(GrantedAbilityHandles[ItemID]);
		// 맵에서도 핸들 정보를 제거합니다.
		GrantedAbilityHandles.Remove(ItemID);
	}

	// 인벤토리 배열에서 아이템 ID를 제거합니다. (첫 번째 발견된 것 하나만)
	InventoryItems.RemoveSingle(ItemID);
	// UI 갱신을 위해 델리게이트를 방송합니다.
	OnInventoryUpdated.Broadcast();

	// 로그 출력
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("Item Removed: %s"), *ItemID.ToString()));
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set! Cannot add random item."));
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("InventoryComponent: No Common grade items found in ItemDataTable!"));
	}
	// ------------------
}

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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("CombineItemByResultID called on Client!"));
		return false;
	}
	// RecipeDataTable이 설정되어 있는지 확인합니다.
	if (!RecipeDataTable)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RecipeDataTable is NOT set!"));
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("No recipe found for ResultItemID: %s"), *ResultItemID.ToString()));
		return false;
	}

	// 2. 이 조합법으로 조합이 가능한지(재료가 충분한지) CanCombine 함수로 확인합니다.
	// (CanCombine 함수는 내부적으로 RecipeID를 사용하므로, 찾은 조합법의 실제 Row Name을 넘겨줘야 할 수도 있습니다.
	// 여기서는 임시로 ResultItemID를 RecipeID처럼 사용합니다. 필요시 수정 필요)
	if (!CanCombine(ResultItemID)) // TODO: RecipeData->GetRowName() 같은 함수가 있다면 그것을 사용
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Cannot combine %s: Ingredients missing."), *ResultItemID.ToString()));
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
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("CRITICAL ERROR in CombineItem: Ingredient %s vanished AFTER CanCombine passed!"), *IngredientID.ToString()));
			break; // 재료 소모 중단
		}
	}

	// 4. 모든 재료가 성공적으로 소모되었다면, 결과 아이템을 인벤토리에 추가합니다.
	if (!bFailedToRemoveIngredient)
	{
		AddItem(RecipeData->ResultItemID); // 결과 아이템 추가 및 효과 적용
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Successfully combined: %s"), *RecipeData->ResultItemID.ToString()));
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