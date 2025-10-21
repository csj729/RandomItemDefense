#include "InventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "GameplayTagsManager.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// �� ������Ʈ ��ü�� ������ �ʿ䰡 �����ϴ�. (�������� ����)
	// �� ������Ʈ�� �����ϴ� GameplayEffect�� Ability�� ������ �����˴ϴ�.
}

/**
 * @brief ASC ������ �ʱ�ȭ�մϴ�.
 */
void UInventoryComponent::Initialize(UAbilitySystemComponent* InASC)
{
	AbilitySystemComponent = InASC;
}

/**
 * @brief ������ ���̺��� ������ ������ ã���ϴ�.
 */
const FItemData* UInventoryComponent::FindItemDataByID(FName ItemID) const
{
	if (!ItemDataTable)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set!"));
		return nullptr;
	}
	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("InventoryComponent::FindItemDataByID"));
}

FGameplayTag UInventoryComponent::GetTagFromStatType(EItemStatType StatType) const
{
	// 2�ܰ迡�� .ini ���Ͽ� ����� �±׸� ���⼭ ����մϴ�.
	// .ini ���Ͽ� ����� �±� �̸����Դϴ�.
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
	case EItemStatType::StunChance:
		return FGameplayTag::RequestGameplayTag(FName("Item.Stat.StunChance"));

	default:
		return FGameplayTag::EmptyTag;
	}
}

/**
 * @brief (���� ����) ������ �߰� �� GAS ȿ�� ����
 */
void UInventoryComponent::AddItem(FName ItemID)
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;

	const FItemData* ItemData = FindItemDataByID(ItemID);
	if (!ItemData)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("InventoryComponent: ItemID '%s' not found."), *ItemID.ToString()));
		return;
	}

	// 1. ���� ȿ��(GE) ���� (SetByCaller)
	// �� �������� BaseStats�� ������ �ְ�, �츮�� C++�� GenericItemStatEffect�� �����ߴٸ�
	if (ItemData->BaseStats.Num() > 0 && GenericItemStatEffect)
	{
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GenericItemStatEffect, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// ������ ���̺��� �о�� ��� ������ ��ȸ�ϸ� SetByCaller ���� �����մϴ�.
			for (const FItemStatData& StatData : ItemData->BaseStats)
			{
				FGameplayTag StatTag = GetTagFromStatType(StatData.StatType);
				if (StatTag.IsValid())
				{
					// "Item.Stat.AttackDamage" �±׿� 10.0f ���� ����
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(StatTag, StatData.Value);
				}
			}

			// ��� SetByCaller ���� ���Ե� *�ϳ���* GE Spec�� �����մϴ�.
			FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			AppliedStatEffectHandles.Add(ItemID, ActiveHandle); // �ڵ� ����
		}
	}
	// ----------------------------------------

	// 2. ���� �ɷ�(GA) �ο� (������ ����)
	if (ItemData->GrantedAbility)
	{
		FGameplayAbilitySpec AbilitySpec(ItemData->GrantedAbility);
		FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		GrantedAbilityHandles.Add(ItemID, AbilityHandle);
	}

	InventoryItems.Add(ItemID);
	OnInventoryUpdated.Broadcast(); // UI ���� �˸�

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Item Added (SBC): %s"), *ItemID.ToString()));
}

/**
 * @brief (���� ����) ������ ���� �� GAS ȿ�� ����
 */
void UInventoryComponent::RemoveItem(FName ItemID)
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;
	if (!InventoryItems.Contains(ItemID)) return;

	// 1. ���� ȿ��(GE) ���� (SBC�� ����� �ڵ� ����)
	if (AppliedStatEffectHandles.Contains(ItemID))
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(AppliedStatEffectHandles[ItemID]);
		AppliedStatEffectHandles.Remove(ItemID);
	}

	// 2. ���� �ɷ�(GA) ���� (������ ����)
	if (GrantedAbilityHandles.Contains(ItemID))
	{
		AbilitySystemComponent->ClearAbility(GrantedAbilityHandles[ItemID]);
		GrantedAbilityHandles.Remove(ItemID);
	}

	InventoryItems.RemoveSingle(ItemID);
	OnInventoryUpdated.Broadcast(); // UI ���� �˸�

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("Item Removed: %s"), *ItemID.ToString()));
}

/**
 * @brief (���� ����) '������ �̱�'
 */
void UInventoryComponent::AddRandomItem()
{
	if (!ItemDataTable)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set! Cannot add random item."));
		return;
	}

	// ������ ���̺��� ��� ������ ID(Row Name)�� �����ɴϴ�.
	TArray<FName> ItemIDs = ItemDataTable->GetRowNames();
	if (ItemIDs.Num() == 0) return;

	// (�ӽ�) ��ȹ���� '����' ��޸� �̵��� ���͸��� �ʿ�������,
	// ������ ���̺��� ��� ������ �� �ϳ��� �������� �̽��ϴ�.
	FName RandomItemID = ItemIDs[FMath::RandRange(0, ItemIDs.Num() - 1)];

	// AddItem �Լ��� ȣ���Ͽ� ���� �κ��丮�� �߰��մϴ�.
	AddItem(RandomItemID);
}