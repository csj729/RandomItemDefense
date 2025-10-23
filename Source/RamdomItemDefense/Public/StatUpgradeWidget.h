#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "StatUpgradeWidget.generated.h"

struct FOnAttributeChangeData;

// ���� ����
class AMyPlayerState;
class UTextBlock;
class UButton;
class UAbilitySystemComponent; // ASC ���� ����

UCLASS()
class RAMDOMITEMDEFENSE_API UStatUpgradeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** ���� ���� �� ȣ�� (�������Ʈ�� Event Construct) */
	virtual void NativeConstruct() override;

	/** PlayerState ���� */
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	/** ĳ������ AbilitySystemComponent ���� (���� ���� �� ���������) */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> OwningASC;

	// --- UMG ���� ���ε� ---
	// UMG �����̳ʿ��� ���� �̸��� IsVariable? üũ�� ��Ȯ�� ��ġ���Ѿ� �մϴ�.
protected:
	/** ���� ��� ǥ�� �ؽ�Ʈ (UMG �̸�: GoldText) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;

	// --- ���ݷ� ���� ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_ValueText; // ���� ��
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> AtkDmg_UpgradeButton;

	// --- ���� �ӵ� ���� ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> AtkSpd_UpgradeButton;

	// --- ġ��Ÿ ���� ���� ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CritDmg_UpgradeButton;

	// --- ���� ���� ���� ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_ChanceText; // ���� Ȯ��
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ArmorReduction_UpgradeButton;

	// --- ��ų �ߵ� Ȯ�� ���� ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_ChanceText; // ���� Ȯ��
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SkillChance_UpgradeButton;

	// (�ʿ�� �ٸ� ��� ��ȭ �Ұ� ���� ���ε� �߰� ���� - �� ǥ�ÿ�)
	// ��: ġ��Ÿ Ȯ�� ǥ�ÿ�
	// UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CritChance_ValueText; // Optional: ��� ������ ���� �� ��

// --- UI ������Ʈ �Լ� ---
protected:
	/** PlayerState�� ��� ���� �� ȣ��˴ϴ�. */
	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

	/** PlayerState�� ���� ���� ���� �� ȣ��˴ϴ�. */
	UFUNCTION()
	void HandleStatLevelChanged(EItemStatType StatType, int32 NewLevel);

	/** Ư�� ���� ������ UI(����, ���, Ȯ��, ��ư Ȱ��ȭ)�� ������Ʈ�մϴ�. */
	void UpdateStatLineUI(EItemStatType StatType);

	/** ��� ���� ������ UI�� ������Ʈ�մϴ�. */
	void UpdateAllStatLines();

	// --- ���� ���� �� ���� �ڵ鷯 ���� ---
	// ASC�� Attribute ���� ����� �� ȣ��˴ϴ�.
	void HandleAttackDamageChanged(const FOnAttributeChangeData& Data);
	void HandleAttackSpeedChanged(const FOnAttributeChangeData& Data);
	void HandleCritDamageChanged(const FOnAttributeChangeData& Data);
	void HandleArmorReductionChanged(const FOnAttributeChangeData& Data);
	void HandleSkillChanceChanged(const FOnAttributeChangeData& Data);
	// (�ʿ�� �ٸ� ���� �ڵ鷯�� ����, ��: HandleCritChanceChanged)


// --- ��ư Ŭ�� �ڵ鷯 ---
protected:
	// �� ��ư Ŭ�� �� ȣ��� UFUNCTION�� (PlayerState�� ���� RPC ȣ��)
	UFUNCTION() void HandleUpgradeAtkDmg();
	UFUNCTION() void HandleUpgradeAtkSpd();
	UFUNCTION() void HandleUpgradeCritDmg();
	UFUNCTION() void HandleUpgradeArmorReduction();
	UFUNCTION() void HandleUpgradeSkillChance();
};