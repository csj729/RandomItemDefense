#include "InventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h" // MakeEffectContext
#include "Engine/DataTable.h"
#include "Engine/Engine.h" // GEngine ����� �޽���
#include "GameplayTagsManager.h" // GameplayTag ����

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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set!"));
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

/**
 * @brief (���� ����) �κ��丮�� �������� �߰��ϰ� GAS ȿ��(SetByCaller) �� �����Ƽ�� �����մϴ�.
 */
void UInventoryComponent::AddItem(FName ItemID)
{
	// ASC�� ��ȿ����, �׸��� �������� ����Ǵ��� Ȯ���մϴ�.
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent::AddItem called on Client!"));
		return;
	}

	// ������ ���̺��� ������ ������ �����ɴϴ�.
	bool bFound = false;
	const FItemData ItemData = GetItemData(ItemID, bFound);
	if (!bFound)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("InventoryComponent: ItemID '%s' not found in ItemDataTable."), *ItemID.ToString()));
		return;
	}

	// 1. ���� ȿ��(GE) ���� (SetByCaller ���)
	// ������ �����Ϳ� BaseStats ������ �ְ�, GenericItemStatEffect�� �����Ǿ� �ִ��� Ȯ���մϴ�.
	if (ItemData.BaseStats.Num() > 0 && GenericItemStatEffect)
	{
		// GameplayEffect ���ؽ�Ʈ�� �����մϴ�.
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this); // ����Ʈ �����ڸ� InventoryComponent�� ����

		// GenericItemStatEffect�� ������� Spec �ڵ��� �����մϴ�.
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GenericItemStatEffect, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// ������ �������� ��� BaseStats�� ��ȸ�ϸ� SetByCaller ���� �����մϴ�.
			for (const FItemStatData& StatData : ItemData.BaseStats)
			{
				FGameplayTag StatTag = GetTagFromStatType(StatData.StatType); // ���� Ÿ�Կ� �´� �±� ��������
				if (StatTag.IsValid())
				{
					// Spec �����Ϳ� SetByCaller ���� �����մϴ� (��: "Item.Stat.AttackDamage" �±׿� 10.0f)
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(StatTag, StatData.Value);
				}
			}

			// ��� ���� ������ Spec�� ĳ���� �ڽſ��� �����մϴ�.
			FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			// ���߿� ������ �� �ֵ��� �ڵ��� ������ ID�� �Բ� �ʿ� �����մϴ�.
			AppliedStatEffectHandles.Add(ItemID, ActiveHandle);
		}
	}

	// 2. ���� �ɷ�(GA) �ο� (GameplayAbility)
	// ������ �����Ϳ� GrantedAbility�� �����Ǿ� �ִ��� Ȯ���մϴ�.
	if (ItemData.GrantedAbility)
	{
		// �����Ƽ Spec�� �����ϰ� ĳ���Ϳ��� �ο��մϴ�.
		FGameplayAbilitySpec AbilitySpec(ItemData.GrantedAbility);
		FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		// ���߿� ������ �� �ֵ��� �ڵ��� ������ ID�� �Բ� �ʿ� �����մϴ�.
		GrantedAbilityHandles.Add(ItemID, AbilityHandle);
	}

	// �κ��丮 �迭�� ������ ID�� �߰��մϴ�.
	InventoryItems.Add(ItemID);
	// UI ������ ���� ��������Ʈ�� ����մϴ�.
	OnInventoryUpdated.Broadcast();

	// �α� ���
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Item Added (SBC): %s"), *ItemID.ToString()));
}

/**
 * @brief (���� ����) �κ��丮���� �������� �����ϰ� GAS ȿ�� �� �����Ƽ�� �����մϴ�.
 */
void UInventoryComponent::RemoveItem(FName ItemID)
{
	// ASC ��ȿ��, ���� ����, ������ ���� ���θ� Ȯ���մϴ�.
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;
	if (!InventoryItems.Contains(ItemID)) return;

	// 1. ���� ȿ��(GE) ����
	// �ش� ������ ID�� ����� GE �ڵ��� �ִ��� Ȯ���մϴ�.
	if (AppliedStatEffectHandles.Contains(ItemID))
	{
		// �ڵ��� ����Ͽ� ����� GameplayEffect�� �����մϴ�.
		AbilitySystemComponent->RemoveActiveGameplayEffect(AppliedStatEffectHandles[ItemID]);
		// �ʿ����� �ڵ� ������ �����մϴ�.
		AppliedStatEffectHandles.Remove(ItemID);
	}

	// 2. ���� �ɷ�(GA) ����
	// �ش� ������ ID�� ����� GA �ڵ��� �ִ��� Ȯ���մϴ�.
	if (GrantedAbilityHandles.Contains(ItemID))
	{
		// �ڵ��� ����Ͽ� �ο��� GameplayAbility�� �����մϴ�.
		AbilitySystemComponent->ClearAbility(GrantedAbilityHandles[ItemID]);
		// �ʿ����� �ڵ� ������ �����մϴ�.
		GrantedAbilityHandles.Remove(ItemID);
	}

	// �κ��丮 �迭���� ������ ID�� �����մϴ�. (ù ��° �߰ߵ� �� �ϳ���)
	InventoryItems.RemoveSingle(ItemID);
	// UI ������ ���� ��������Ʈ�� ����մϴ�.
	OnInventoryUpdated.Broadcast();

	// �α� ���
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("Item Removed: %s"), *ItemID.ToString()));
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set! Cannot add random item."));
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("InventoryComponent: No Common grade items found in ItemDataTable!"));
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("CombineItemByResultID called on Client!"));
		return false;
	}
	// RecipeDataTable�� �����Ǿ� �ִ��� Ȯ���մϴ�.
	if (!RecipeDataTable)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RecipeDataTable is NOT set!"));
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("No recipe found for ResultItemID: %s"), *ResultItemID.ToString()));
		return false;
	}

	// 2. �� ���չ����� ������ ��������(��ᰡ �������) CanCombine �Լ��� Ȯ���մϴ�.
	// (CanCombine �Լ��� ���������� RecipeID�� ����ϹǷ�, ã�� ���չ��� ���� Row Name�� �Ѱ���� �� ���� �ֽ��ϴ�.
	// ���⼭�� �ӽ÷� ResultItemID�� RecipeIDó�� ����մϴ�. �ʿ�� ���� �ʿ�)
	if (!CanCombine(ResultItemID)) // TODO: RecipeData->GetRowName() ���� �Լ��� �ִٸ� �װ��� ���
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Cannot combine %s: Ingredients missing."), *ResultItemID.ToString()));
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
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("CRITICAL ERROR in CombineItem: Ingredient %s vanished AFTER CanCombine passed!"), *IngredientID.ToString()));
			break; // ��� �Ҹ� �ߴ�
		}
	}

	// 4. ��� ��ᰡ ���������� �Ҹ�Ǿ��ٸ�, ��� �������� �κ��丮�� �߰��մϴ�.
	if (!bFailedToRemoveIngredient)
	{
		AddItem(RecipeData->ResultItemID); // ��� ������ �߰� �� ȿ�� ����
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Successfully combined: %s"), *RecipeData->ResultItemID.ToString()));
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