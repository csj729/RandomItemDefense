#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "StatUpgradeWidget.generated.h"

struct FOnAttributeChangeData;
class AMyPlayerState;
class UTextBlock;
class UButton;
class UAbilitySystemComponent;

UCLASS()
class RAMDOMITEMDEFENSE_API UStatUpgradeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	void BindPlayerState();

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> OwningASC;

	// --- [ UMG Bindings ] ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> GoldText;

	// [Attack Damage]
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> AtkDmg_UpgradeButton;

	// [Attack Speed]
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> AtkSpd_UpgradeButton;

	// [Crit Damage]
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CritDmg_UpgradeButton;

	// [Armor Reduction]
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_ChanceText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ArmorReduction_UpgradeButton;

	// [Skill Chance]
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_ChanceText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SkillChance_UpgradeButton;

	// --- [ Logic ] ---
	UFUNCTION() void HandleGoldChanged(int32 NewGold);
	UFUNCTION() void HandleStatLevelChanged(EItemStatType StatType, int32 NewLevel);

	void UpdateStatLineUI(EItemStatType StatType);
	void UpdateAllStatLines();
	void DisableAllUpgradeButtons();

	// ASC Change Handlers
	void HandleAttackDamageChanged(const FOnAttributeChangeData& Data);
	void HandleAttackSpeedChanged(const FOnAttributeChangeData& Data);
	void HandleCritDamageChanged(const FOnAttributeChangeData& Data);
	void HandleArmorReductionChanged(const FOnAttributeChangeData& Data);
	void HandleSkillChanceChanged(const FOnAttributeChangeData& Data);

	// Button Click Handlers
	UFUNCTION() void HandleUpgradeAtkDmg();
	UFUNCTION() void HandleUpgradeAtkSpd();
	UFUNCTION() void HandleUpgradeCritDmg();
	UFUNCTION() void HandleUpgradeArmorReduction();
	UFUNCTION() void HandleUpgradeSkillChance();
};