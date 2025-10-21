#include "InventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "GameplayTagsManager.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// 이 컴포넌트 자체는 복제할 필요가 없습니다. (서버에만 존재)
	// 이 컴포넌트가 적용하는 GameplayEffect와 Ability가 스스로 복제됩니다.
}

/**
 * @brief ASC 참조를 초기화합니다.
 */
void UInventoryComponent::Initialize(UAbilitySystemComponent* InASC)
{
	AbilitySystemComponent = InASC;
}

/**
 * @brief 데이터 테이블에서 아이템 정보를 찾습니다.
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
	// 2단계에서 .ini 파일에 등록한 태그를 여기서 사용합니다.
	// .ini 파일에 등록할 태그 이름들입니다.
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
 * @brief (서버 전용) 아이템 추가 및 GAS 효과 적용
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

	// 1. 스탯 효과(GE) 적용 (SetByCaller)
	// 이 아이템이 BaseStats를 가지고 있고, 우리가 C++에 GenericItemStatEffect를 지정했다면
	if (ItemData->BaseStats.Num() > 0 && GenericItemStatEffect)
	{
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GenericItemStatEffect, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 데이터 테이블에서 읽어온 모든 스탯을 순회하며 SetByCaller 값을 주입합니다.
			for (const FItemStatData& StatData : ItemData->BaseStats)
			{
				FGameplayTag StatTag = GetTagFromStatType(StatData.StatType);
				if (StatTag.IsValid())
				{
					// "Item.Stat.AttackDamage" 태그에 10.0f 값을 설정
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(StatTag, StatData.Value);
				}
			}

			// 모든 SetByCaller 값이 주입된 *하나의* GE Spec을 적용합니다.
			FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			AppliedStatEffectHandles.Add(ItemID, ActiveHandle); // 핸들 저장
		}
	}
	// ----------------------------------------

	// 2. 고유 능력(GA) 부여 (기존과 동일)
	if (ItemData->GrantedAbility)
	{
		FGameplayAbilitySpec AbilitySpec(ItemData->GrantedAbility);
		FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		GrantedAbilityHandles.Add(ItemID, AbilityHandle);
	}

	InventoryItems.Add(ItemID);
	OnInventoryUpdated.Broadcast(); // UI 갱신 알림

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Item Added (SBC): %s"), *ItemID.ToString()));
}

/**
 * @brief (서버 전용) 아이템 제거 및 GAS 효과 해제
 */
void UInventoryComponent::RemoveItem(FName ItemID)
{
	if (!AbilitySystemComponent.IsValid()) return;
	if (!GetOwner()->HasAuthority()) return;
	if (!InventoryItems.Contains(ItemID)) return;

	// 1. 스탯 효과(GE) 제거 (SBC로 적용된 핸들 제거)
	if (AppliedStatEffectHandles.Contains(ItemID))
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(AppliedStatEffectHandles[ItemID]);
		AppliedStatEffectHandles.Remove(ItemID);
	}

	// 2. 고유 능력(GA) 제거 (기존과 동일)
	if (GrantedAbilityHandles.Contains(ItemID))
	{
		AbilitySystemComponent->ClearAbility(GrantedAbilityHandles[ItemID]);
		GrantedAbilityHandles.Remove(ItemID);
	}

	InventoryItems.RemoveSingle(ItemID);
	OnInventoryUpdated.Broadcast(); // UI 갱신 알림

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("Item Removed: %s"), *ItemID.ToString()));
}

/**
 * @brief (서버 전용) '아이템 뽑기'
 */
void UInventoryComponent::AddRandomItem()
{
	if (!ItemDataTable)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryComponent: ItemDataTable is NOT set! Cannot add random item."));
		return;
	}

	// 데이터 테이블의 모든 아이템 ID(Row Name)를 가져옵니다.
	TArray<FName> ItemIDs = ItemDataTable->GetRowNames();
	if (ItemIDs.Num() == 0) return;

	// (임시) 기획서의 '흔함' 등급만 뽑도록 필터링이 필요하지만,
	// 지금은 테이블의 모든 아이템 중 하나를 무작위로 뽑습니다.
	FName RandomItemID = ItemIDs[FMath::RandRange(0, ItemIDs.Num() - 1)];

	// AddItem 함수를 호출하여 실제 인벤토리에 추가합니다.
	AddItem(RandomItemID);
}