// csj729/randomitemdefense/RandomItemDefense-78a128504f0127dc02646504d4a1e1c677a0e811/Source/RamdomItemDefense/Private/InventoryComponent.cpp

#include "InventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h" // MakeEffectContext
#include "Engine/DataTable.h"
#include "Engine/Engine.h" // GEngine ����� �޽���
#include "GameplayTagsManager.h" // GameplayTag ����
#include "RamdomItemDefense.h" // RID_LOG ��ũ�ο�

UInventoryComponent::UInventoryComponent()
{
	// �� ������Ʈ�� �������� �����ϹǷ� Tick�� �ʿ� �����ϴ�.
	PrimaryComponentTick.bCanEverTick = false;
	// ���� ������ �ʿ� �����ϴ�.
}

void UInventoryComponent::Initialize(UAbilitySystemComponent* InASC)
{
	AbilitySystemComponent = InASC;
}

/**
 * @brief ������ ���̺��� ������ ������ ã�� ���纻�� ��ȯ�մϴ�.
 * @param ItemID ������ ������ ID
 * @param bSuccess ������ �����͸� ���������� ã�Ҵ��� ���� (���)
 * @return ������ ������ (���纻), ã�� ���ϸ� �⺻��
 */
FItemData UInventoryComponent::GetItemData(FName ItemID, bool& bSuccess) const
{
	bSuccess = false; // �⺻������ ���з� ����
	if (!ItemDataTable)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set!"));
		// -----------------------------------------
		return FItemData(); // �� ����ü ��ȯ
	}

	// ������ ���̺��� ItemID(Row Name)�� �ش��ϴ� FItemData ���� ã���ϴ�.
	FItemData* Row = ItemDataTable->FindRow<FItemData>(ItemID, TEXT("InventoryComponent::GetItemData"));
	if (Row)
	{
		bSuccess = true; // ã������ �������� ����
		return *Row;     // ã�� �������� ���纻(*)�� ��ȯ
	}

	// ã�� �������� �� ����ü�� ��ȯ�մϴ�.
	return FItemData();
}

/**
 * @brief EItemStatType �������� SetByCaller�� FGameplayTag�� ��ȯ�մϴ�.
 */
FGameplayTag UInventoryComponent::GetTagFromStatType(EItemStatType StatType) const
{
	// .ini ���Ͽ� ����� �±� �̸����� ����մϴ�.
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
	case EItemStatType::StunChance: // MyAttributeSet�� �߰��Ǿ��ٰ� ����
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.StunChance"));
	default:
		return FGameplayTag::EmptyTag; // ��ȿ���� ���� Ÿ���̸� �� �±� ��ȯ
	}
}

/** (���� ����) ������ �߰� */
void UInventoryComponent::AddItem(FName ItemID)
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;

	bool bFound = false;
	const FItemData ItemData = GetItemData(ItemID, bFound);
	if (!bFound) return;

	// 1. ���� ȿ��(GE) ���� (SetByCaller)
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

			// --- [����� �α� �߰�] ---
			if (ActiveHandle.IsValid())
			{
				// --- [�ڵ� ����] TMap::Add -> TArray::Add ---
				ActiveStatEffects.Add({ ItemID, ActiveHandle });
				// -----------------------------------------
				RID_LOG(FColor::Cyan, TEXT("AddItem: Stored GE Handle for %s"), *ItemID.ToString());
			}
			else
			{
				RID_LOG(FColor::Red, TEXT("AddItem: Applied GE Handle for %s is INVALID!"), *ItemID.ToString());
			}
		}
	}

	// 2. ���� �ɷ�(GA) �ο� (���� ����)
	if (ItemData.GrantedAbility)
	{
		FGameplayAbilitySpec AbilitySpec(ItemData.GrantedAbility);
		FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);

		// --- [�ڵ� ����] TMap::Add -> TArray::Add ---
		if (AbilityHandle.IsValid())
		{
			ActiveGrantedAbilities.Add({ ItemID, AbilityHandle });
			RID_LOG(FColor::Cyan, TEXT("AddItem: Stored GA Handle for %s"), *ItemID.ToString());
		}
		else
		{
			RID_LOG(FColor::Red, TEXT("AddItem: Applied GA Handle for %s is INVALID!"), *ItemID.ToString());
		}
		// --- [�ڵ� ���� ��] ---
	}

	InventoryItems.Add(ItemID);
	OnInventoryUpdated.Broadcast();
	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	RID_LOG(FColor::Green, TEXT("Item Added (SBC): %s"), *ItemID.ToString());
	// -----------------------------------------
}

/** (���� ����) ������ ���� */
void UInventoryComponent::RemoveItem(FName ItemID)
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;

	// --- [�ڵ� ����] TMap::Contains -> TArray::IndexOfByPredicate ---

	// 1. ���� ȿ��(GE) ����
	// ItemID�� ��ġ�ϴ� 'ù ��°' ���� ȿ�� �׸��� �迭���� ã���ϴ�.
	int32 StatEffectIndex = ActiveStatEffects.IndexOfByPredicate(
		[ItemID](const FActiveItemStatEffect& Effect) { return Effect.ItemID == ItemID; }
	);

	// �׸��� ã�Ҵٸ�
	if (StatEffectIndex != INDEX_NONE)
	{
		// �ڵ��� �����ɴϴ�.
		FActiveGameplayEffectHandle HandleToRemove = ActiveStatEffects[StatEffectIndex].Handle;
		if (HandleToRemove.IsValid())
		{
			// ASC���� GE�� �����մϴ�.
			bool bRemoved = AbilitySystemComponent->RemoveActiveGameplayEffect(HandleToRemove);
			if (bRemoved)
			{
				RID_LOG(FColor::Orange, TEXT("RemoveItem: Removed GE Handle for %s (Success)"), *ItemID.ToString());
			}
			else
			{
				RID_LOG(FColor::Red, TEXT("RemoveItem: Failed to remove GE Handle for %s"), *ItemID.ToString());
			}
		}
		else
		{
			RID_LOG(FColor::Red, TEXT("RemoveItem: Found INVALID GE Handle in array for %s"), *ItemID.ToString());
		}

		// '�ݵ��' ���� �迭���� �� �׸��� �����մϴ�. (���� RemoveItem ȣ�� �� �� ���� �׸��� ã����)
		ActiveStatEffects.RemoveAt(StatEffectIndex);
	}
	else
	{
		// ������ ���� �������� �� �����Ƿ� ��� ���
		RID_LOG(FColor::Yellow, TEXT("RemoveItem: No GE Handle found in array for %s"), *ItemID.ToString());
	}
	// --- [�ڵ� ���� ��] ---


	// 2. ���� �ɷ�(GA) ����
	// --- [�ڵ� ����] TMap::Contains -> TArray::IndexOfByPredicate ---

	// ItemID�� ��ġ�ϴ� 'ù ��°' �����Ƽ �׸��� �迭���� ã���ϴ�.
	int32 AbilityIndex = ActiveGrantedAbilities.IndexOfByPredicate(
		[ItemID](const FActiveItemAbility& Ability) { return Ability.ItemID == ItemID; }
	);

	// �׸��� ã�Ҵٸ�
	if (AbilityIndex != INDEX_NONE)
	{
		// �ڵ��� �����ɴϴ�.
		FGameplayAbilitySpecHandle HandleToRemove = ActiveGrantedAbilities[AbilityIndex].Handle;
		if (HandleToRemove.IsValid())
		{
			// ASC���� �����Ƽ�� �����մϴ�.
			AbilitySystemComponent->ClearAbility(HandleToRemove);
			RID_LOG(FColor::Orange, TEXT("RemoveItem: Removed GA Handle for %s (Success)"), *ItemID.ToString());
		}
		else
		{
			RID_LOG(FColor::Red, TEXT("RemoveItem: Found INVALID GA Handle in array for %s"), *ItemID.ToString());
		}

		// '�ݵ��' ���� �迭���� �� �׸��� �����մϴ�.
		ActiveGrantedAbilities.RemoveAt(AbilityIndex);
	}
	else
	{
		// �����Ƽ�� ���� �������� �� �����Ƿ� ��� ���
		RID_LOG(FColor::Yellow, TEXT("RemoveItem: No GA Handle found in array for %s"), *ItemID.ToString());
	}
	// --- [�ڵ� ���� ��] ---


	// 3. ���� �κ��丮 �迭���� ���� (���� �������� ����)
	bool bRemovedFromArray = InventoryItems.RemoveSingle(ItemID) > 0;
	if (!bRemovedFromArray) // �������� �迭�� ���µ� ���� �õ��� ���
	{
		RID_LOG(FColor::Red, TEXT("RemoveItem: ItemID %s was not found in InventoryItems array!"), *ItemID.ToString());
		return; // �������� �������Ƿ� UI ������Ʈ ���ʿ�
	}
	// --------------------------------------------------------

	OnInventoryUpdated.Broadcast(); // UI ���� �˸�
	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	RID_LOG(FColor::Yellow, TEXT("Item Removed: %s"), *ItemID.ToString());
	// -----------------------------------------
}

/**
 * @brief (���� ����) ������ ������ ���̺��� ������ �������� �ϳ� �̾� �߰��մϴ�.
 * ('������ �̱�' ���� �� MyPlayerState���� ȣ��)
 */
void UInventoryComponent::AddRandomItem()
{
	// ItemDataTable ��ȿ�� �˻�
	if (!ItemDataTable)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set! Cannot add random item."));
		// -----------------------------------------
		return;
	}

	// ������ ���̺��� ��� �� �̸�(������ ID)�� �����ɴϴ�.
	TArray<FName> AllItemIDs = ItemDataTable->GetRowNames();
	if (AllItemIDs.Num() == 0) return; // �������� ������ ����

	// --- [�ڵ� ����] ---
	// ���� ��� ������ ID�� ������ �� �迭�� ����ϴ�.
	TArray<FName> CommonItemIDs;

	// ��� ������ ID�� ��ȸ�մϴ�.
	for (const FName& ItemID : AllItemIDs)
	{
		bool bSuccess = false;
		// �ش� ������ ID�� �����͸� �����ɴϴ�.
		FItemData ItemData = GetItemData(ItemID, bSuccess);
		// �����͸� ���������� �����԰�, ����� 'Common'���� Ȯ���մϴ�.
		if (bSuccess && ItemData.Grade == EItemGrade::Common)
		{
			// ������ �����ϸ� CommonItemIDs �迭�� �߰��մϴ�.
			CommonItemIDs.Add(ItemID);
		}
	}

	// ���� ��� �������� �ϳ��� �ִ��� Ȯ���մϴ�.
	if (CommonItemIDs.Num() > 0)
	{
		// CommonItemIDs �迭���� ������ �ε����� �����մϴ�.
		int32 RandomIndex = FMath::RandRange(0, CommonItemIDs.Num() - 1);
		FName RandomCommonItemID = CommonItemIDs[RandomIndex];

		// ���õ� ���� ��� ������ ID�� AddItem �Լ��� ȣ���մϴ�.
		AddItem(RandomCommonItemID);
	}
	else
	{
		// ������ ���̺� ���� ��� �������� �ϳ��� ���� ��� �α� ���
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Yellow, TEXT("InventoryComponent: No Common grade items found in ItemDataTable!"));
		// -----------------------------------------
	}
	// ------------------
}

/**
 * @brief (���� ���� / UI ȣ���) ��� ������ ID�� ������� ������ ������ �õ��մϴ�.
 * @param ResultItemID ���� ��� �������� ID (DT_Recipes�� Row Name �Ǵ� ResultItemID �ʵ�)
 * @return ���� ���� ����
 */
bool UInventoryComponent::CombineItemByResultID(FName ResultItemID)
{
	// ���������� ����ǵ��� �մϴ�.
	if (!GetOwner()->HasAuthority())
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("CombineItemByResultID called on Client!"));
		// -----------------------------------------
		return false;
	}
	// RecipeDataTable�� �����Ǿ� �ִ��� Ȯ���մϴ�.
	if (!RecipeDataTable)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("RecipeDataTable is NOT set!"));
		// -----------------------------------------
		return false;
	}

	// 1. ��� ������ ID�� ���չ� �����͸� ã���ϴ�.
	// ���� Row Name(Name �÷�)�� ResultItemID�� ��ġ�ϴ��� ã���ϴ�.
	FRecipeData* RecipeData = RecipeDataTable->FindRow<FRecipeData>(ResultItemID, TEXT("InventoryComponent::CombineItemByResultID"));

	// Row Name���� �� ã�Ҵٸ�, ��� ���չ��� ��ȸ�ϸ� ResultItemID �ʵ尡 ��ġ�ϴ� ���� ã���ϴ�.
	// (������ ���̺� ���迡 ���� �� �κ��� �ʿ� ���� ���� �ֽ��ϴ�)
	if (!RecipeData)
	{
		TArray<FName> RowNames = RecipeDataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FRecipeData* TempRecipe = RecipeDataTable->FindRow<FRecipeData>(RowName, "");
			if (TempRecipe && TempRecipe->ResultItemID == ResultItemID)
			{
				RecipeData = TempRecipe;
				break; // ã������ ���� ����
			}
		}
	}

	// ���������� ���չ��� ã�� ���ߴٸ� ���и� ��ȯ�մϴ�.
	if (!RecipeData)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Yellow, TEXT("No recipe found for ResultItemID: %s"), *ResultItemID.ToString());
		// -----------------------------------------
		return false;
	}

	// 2. �� ���չ����� ������ ��������(��ᰡ �������) CanCombine �Լ��� Ȯ���մϴ�.
	// (CanCombine �Լ��� ���������� RecipeID�� ����ϹǷ�, ã�� ���չ��� ���� Row Name�� �Ѱ���� �� ���� �ֽ��ϴ�.
	// ���⼭�� �ӽ÷� ResultItemID�� RecipeIDó�� ����մϴ�. �ʿ�� ���� �ʿ�)
	if (!CanCombine(ResultItemID)) // TODO: RecipeData->GetRowName() ���� �Լ��� �ִٸ� �װ��� ���
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Yellow, TEXT("Cannot combine %s: Ingredients missing."), *ResultItemID.ToString());
		// -----------------------------------------
		return false;
	}

	// 3. ��� �����۵��� �κ��丮���� �Ҹ�(����)�մϴ�.
	// �����ϰ� �����ϱ� ����, ���� �� ������ �߻��ϴ��� �����մϴ�.
	bool bFailedToRemoveIngredient = false;
	TArray<FName> TempInventoryForRemovalCheck = InventoryItems; // ���� �κ��丮 ���� (���� üũ��)
	for (const FName& IngredientID : RecipeData->Ingredients)
	{
		// ���纻�� �ش� ��ᰡ �ִ��� ���� Ȯ�� (CanCombine ��� �� ���� ���ɼ� ���)
		if (TempInventoryForRemovalCheck.Contains(IngredientID))
		{
			RemoveItem(IngredientID); // ���� �κ��丮���� ������ ���� �� ȿ�� ����
			TempInventoryForRemovalCheck.RemoveSingle(IngredientID); // ���纻������ ����
		}
		else
		{
			// �� ���� CanCombine() ������ �߸��Ǿ��ų�, �� ���̿� �������� ����� ����Դϴ�.
			bFailedToRemoveIngredient = true;
			// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
			RID_LOG(FColor::Red, TEXT("CRITICAL ERROR in CombineItem: Ingredient %s vanished AFTER CanCombine passed!"), *IngredientID.ToString());
			// -----------------------------------------
			break; // ��� �Ҹ� �ߴ�
		}
	}

	// 4. ��� ��ᰡ ���������� �Ҹ�Ǿ��ٸ�, ��� �������� �κ��丮�� �߰��մϴ�.
	if (!bFailedToRemoveIngredient)
	{
		AddItem(RecipeData->ResultItemID); // ��� ������ �߰� �� ȿ�� ����
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Green, TEXT("Successfully combined: %s"), *RecipeData->ResultItemID.ToString());
		// -----------------------------------------
		return true; // ���� ����
	}

	// ��� �Ҹ� �� ������ �߻��ߴٸ� ���и� ��ȯ�մϴ�.
	// (���: ���⼭ �̹� �Ҹ�� ��Ḧ �ٽ� �ǵ����� �ѹ� ������ �߰��� �� �ֽ��ϴ�)
	return false; // ���� ����
}


/**
 * @brief (Ŭ���̾�Ʈ/����) Ư�� ���չ��� �ʿ��� ��Ḧ ��� ������ �ִ��� Ȯ���մϴ�.
 * @param RecipeID ���չ� ID (������ ���̺��� Row Name �Ǵ� ResultItemID - ���� ���� ����)
 * @return ���� ���� ����
 */
bool UInventoryComponent::CanCombine(FName RecipeID) const
{
	// RecipeDataTable�� �����Ǿ� �ִ��� Ȯ���մϴ�.
	if (!RecipeDataTable) return false;

	// 1. ���չ� �����͸� ã���ϴ�. (CombineItemByResultID�� ������ ���� ���)
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

	// ���չ��� ���ų�, ���չ��� �ʿ��� ��� ����� ��������� ���� �Ұ�
	if (!RecipeData || RecipeData->Ingredients.Num() == 0)
	{
		return false;
	}

	// 2. ���� �κ��丮 ������ ���纻�� ����ϴ�. (���� �κ��丮�� �������� ����)
	TArray<FName> TempInventory = InventoryItems;

	// 3. ���չ��� ��� ���(Ingredients)�� ��ȸ�մϴ�.
	for (const FName& IngredientID : RecipeData->Ingredients)
	{
		// ���纻 �κ��丮�� �ش� ��ᰡ �ִ��� Ȯ���մϴ�.
		if (TempInventory.Contains(IngredientID))
		{
			// �ִٸ�, ���纻���� �ش� ��Ḧ �ϳ� �����մϴ�. (���� üũ ����)
			TempInventory.RemoveSingle(IngredientID);
		}
		else
		{
			// ��� �� �ϳ��� ���纻�� ���ٸ�, ��� false(���� �Ұ�)�� ��ȯ�մϴ�.
			return false;
		}
	}

	// 4. ������ ������ ����ߴٸ� (��� ��ᰡ ���纻�� �����ߴٸ�), true(���� ����)�� ��ȯ�մϴ�.
	return true;
}