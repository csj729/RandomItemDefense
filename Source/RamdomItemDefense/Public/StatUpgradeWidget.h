#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTypes.h"
#include "StatUpgradeWidget.generated.h"

struct FOnAttributeChangeData;

// 전방 선언
class AMyPlayerState;
class UTextBlock;
class UButton;
class UAbilitySystemComponent; // ASC 참조 위함

UCLASS()
class RAMDOMITEMDEFENSE_API UStatUpgradeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 위젯 생성 시 호출 (블루프린트의 Event Construct) */
	virtual void NativeConstruct() override;

	/** PlayerState 참조 */
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	/** 캐릭터의 AbilitySystemComponent 참조 (현재 스탯 값 가져오기용) */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> OwningASC;

	// --- UMG 위젯 바인딩 ---
	// UMG 디자이너에서 위젯 이름과 IsVariable? 체크를 정확히 일치시켜야 합니다.
protected:
	/** 현재 골드 표시 텍스트 (UMG 이름: GoldText) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;

	// --- 공격력 라인 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_ValueText; // 현재 값
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkDmg_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> AtkDmg_UpgradeButton;

	// --- 공격 속도 라인 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AtkSpd_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> AtkSpd_UpgradeButton;

	// --- 치명타 피해 라인 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CritDmg_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CritDmg_UpgradeButton;

	// --- 방어력 감소 라인 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ArmorReduction_ChanceText; // 성공 확률
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ArmorReduction_UpgradeButton;

	// --- 스킬 발동 확률 라인 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_LevelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_ValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_CostText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SkillChance_ChanceText; // 성공 확률
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SkillChance_UpgradeButton;

	// (필요시 다른 골드 강화 불가 스탯 라인도 추가 가능 - 값 표시용)
	// 예: 치명타 확률 표시용
	// UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CritChance_ValueText; // Optional: 없어도 컴파일 에러 안 남

// --- UI 업데이트 함수 ---
protected:
	/** PlayerState의 골드 변경 시 호출됩니다. */
	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

	/** PlayerState의 스탯 레벨 변경 시 호출됩니다. */
	UFUNCTION()
	void HandleStatLevelChanged(EItemStatType StatType, int32 NewLevel);

	/** 특정 스탯 라인의 UI(레벨, 비용, 확률, 버튼 활성화)를 업데이트합니다. */
	void UpdateStatLineUI(EItemStatType StatType);

	/** 모든 스탯 라인의 UI를 업데이트합니다. */
	void UpdateAllStatLines();

	// --- 개별 스탯 값 변경 핸들러 선언 ---
	// ASC의 Attribute 값이 변경될 때 호출됩니다.
	void HandleAttackDamageChanged(const FOnAttributeChangeData& Data);
	void HandleAttackSpeedChanged(const FOnAttributeChangeData& Data);
	void HandleCritDamageChanged(const FOnAttributeChangeData& Data);
	void HandleArmorReductionChanged(const FOnAttributeChangeData& Data);
	void HandleSkillChanceChanged(const FOnAttributeChangeData& Data);
	// (필요시 다른 스탯 핸들러도 선언, 예: HandleCritChanceChanged)


// --- 버튼 클릭 핸들러 ---
protected:
	// 각 버튼 클릭 시 호출될 UFUNCTION들 (PlayerState의 서버 RPC 호출)
	UFUNCTION() void HandleUpgradeAtkDmg();
	UFUNCTION() void HandleUpgradeAtkSpd();
	UFUNCTION() void HandleUpgradeCritDmg();
	UFUNCTION() void HandleUpgradeArmorReduction();
	UFUNCTION() void HandleUpgradeSkillChance();
};