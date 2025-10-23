#include "StatUpgradeWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MyPlayerState.h"
#include "MyAttributeSet.h" // Attribute ���� ����
#include "AbilitySystemComponent.h" // ASC ���
#include "RamdomItemDefenseCharacter.h" // ĳ���� Ŭ���� (ASC �������� ����)
#include "GameplayEffectTypes.h"
#include "Engine/Engine.h" // GEngine ����� �޽��� (���� ����)

void UStatUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// PlayerState ���� �������� �� ��������Ʈ ���ε�
	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	if (MyPlayerState)
	{
		MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleGoldChanged);
		MyPlayerState->OnStatLevelChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleStatLevelChanged);

		// �ʱ� UI ������Ʈ
		HandleGoldChanged(MyPlayerState->GetGold());
		UpdateAllStatLines(); // ����, ���, ��ư ���� �ʱ�ȭ
	}
	else
	{
		// PlayerState�� �ʰ� �ε�� ��� ���� ƽ�� ��õ�
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
			if (MyPlayerState)
			{
				MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleGoldChanged);
				MyPlayerState->OnStatLevelChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleStatLevelChanged);
				HandleGoldChanged(MyPlayerState->GetGold());
				UpdateAllStatLines();
				// ASC ���ε��� ���⼭ �ٽ� �õ��ؾ� �� �� ����
				// ... (�Ʒ� ASC ���ε� ���� ����) ...
			}
			});
	}

	// ĳ������ ASC ���� �������� �� ���� ���� ��������Ʈ ���ε�
	ARamdomItemDefenseCharacter* OwningCharacter = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>();
	if (OwningCharacter)
	{
		OwningASC = OwningCharacter->GetAbilitySystemComponent();
		if (OwningASC.IsValid())
		{
			// �� Attribute ���� �� �ش� �ڵ鷯 �Լ��� ȣ���ϵ��� ����
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackDamageAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleAttackDamageChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleAttackSpeedChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritDamageAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleCritDamageChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetArmorReductionAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleArmorReductionChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetSkillActivationChanceAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleSkillChanceChanged);
			// (��� ��ȭ �Ұ� ���� �� ǥ�ð� �ʿ��ϴٸ� ���⼭ ���ε� �߰�)
			// OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritChanceAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleCritChanceChanged); // ����

			// �ʱ� ���� �� UI ������Ʈ
			// �� �ڵ鷯�� ���� ȣ���Ͽ� �ʱ� ���� �����մϴ�.
			FOnAttributeChangeData DummyData; // �ӽ� ������ ����ü
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetAttackDamageAttribute()); HandleAttackDamageChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetAttackSpeedAttribute()); HandleAttackSpeedChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetCritDamageAttribute()); HandleCritDamageChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetArmorReductionAttribute()); HandleArmorReductionChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetSkillActivationChanceAttribute()); HandleSkillChanceChanged(DummyData);
			// (�ٸ� ���� �ʱⰪ ������ �ʿ�� �߰�)
		}
	}


	// ��ư Ŭ�� �̺�Ʈ ���ε�
	if (AtkDmg_UpgradeButton) AtkDmg_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeAtkDmg);
	if (AtkSpd_UpgradeButton) AtkSpd_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeAtkSpd);
	if (CritDmg_UpgradeButton) CritDmg_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeCritDmg);
	if (ArmorReduction_UpgradeButton) ArmorReduction_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeArmorReduction);
	if (SkillChance_UpgradeButton) SkillChance_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeSkillChance);
}

// ��� ���� �� ȣ��
void UStatUpgradeWidget::HandleGoldChanged(int32 NewGold)
{
	if (GoldText)
	{
		// ��� �ؽ�Ʈ ������Ʈ (��: FText::AsCurrency(NewGold, TEXT("G")))
		GoldText->SetText(FText::AsNumber(NewGold));
	}
	// ��尡 ����Ǹ� ��� ��ư�� Ȱ��ȭ ���¸� �ٽ� ���
	UpdateAllStatLines();
}

// ���� ���� ���� �� ȣ�� (PlayerState RepNotify -> Delegate)
void UStatUpgradeWidget::HandleStatLevelChanged(EItemStatType StatType, int32 NewLevel)
{
	// �ش� ���� ������ ����, ���, Ȯ��, ��ư ���� ������Ʈ
	UpdateStatLineUI(StatType);
}

// ��� ���� ���� UI ������Ʈ (�ʱ�ȭ �� ��� ���� ��)
void UStatUpgradeWidget::UpdateAllStatLines()
{
	// ��� ��ȭ ������ ���ȿ� ���� UI ������Ʈ ȣ��
	UpdateStatLineUI(EItemStatType::AttackDamage);
	UpdateStatLineUI(EItemStatType::AttackSpeed);
	UpdateStatLineUI(EItemStatType::CritDamage);
	UpdateStatLineUI(EItemStatType::ArmorReduction);
	UpdateStatLineUI(EItemStatType::SkillActivationChance);
	// (��� ��ȭ �Ұ� ������ ȣ���� �ʿ� ����)
}

/** Ư�� ���� ���� UI ������Ʈ (����, ���, Ȯ��, ��ư ����) */
void UStatUpgradeWidget::UpdateStatLineUI(EItemStatType StatType)
{
	if (!MyPlayerState) return;

	int32 CurrentLevel = MyPlayerState->GetStatLevel(StatType);
	int32 CurrentGold = MyPlayerState->GetGold();

	// --- ��ȭ ��Ģ (PlayerState�� �����ϰ� ����) ---
	const bool bIsBasicStat = (StatType == EItemStatType::AttackDamage || StatType == EItemStatType::AttackSpeed || StatType == EItemStatType::CritDamage);
	const bool bIsSpecialStat = (StatType == EItemStatType::ArmorReduction || StatType == EItemStatType::SkillActivationChance);
	int32 MaxLevel = bIsBasicStat ? 999 : 3;
	int32 BaseCost = 100;
	int32 CostIncreaseFactor = 50;
	float SuccessChance = 1.0f;
	// ---------------------------------------------

	// ���� ���� Ȯ��
	bool bMaxLevelReached = (CurrentLevel >= MaxLevel);

	// ��� ���
	int32 UpgradeCost = BaseCost + (CurrentLevel * CostIncreaseFactor);
	bool bCanAfford = (CurrentGold >= UpgradeCost);

	// Ư�� ���� ���� Ȯ�� ��� (ǥ�ÿ�)
	if (bIsSpecialStat)
	{
		switch (CurrentLevel)
		{
		case 0: SuccessChance = 0.5f; break; // 50%
		case 1: SuccessChance = 0.4f; break; // 40%
		case 2: SuccessChance = 0.3f; break; // 30%
		default: SuccessChance = 0.0f; break; // �ִ� ���� ����
		}
	}

	// UI ��� ������Ʈ (Switch ���)
	FText LevelText = FText::Format(NSLOCTEXT("StatUpgradeWidget", "LevelFormat", "Lv.{0}"), FText::AsNumber(CurrentLevel));
	FText CostText = FText::Format(NSLOCTEXT("StatUpgradeWidget", "CostFormat", "{0} G"), FText::AsNumber(UpgradeCost));
	FText ChanceText = FText::Format(NSLOCTEXT("StatUpgradeWidget", "ChanceFormat", "({0}%)"), FText::AsPercent(SuccessChance));
	bool bButtonEnabled = bCanAfford && !bMaxLevelReached;

	switch (StatType)
	{
	case EItemStatType::AttackDamage:
		if (AtkDmg_LevelText) AtkDmg_LevelText->SetText(LevelText);
		if (AtkDmg_CostText) AtkDmg_CostText->SetText(CostText);
		if (AtkDmg_UpgradeButton) AtkDmg_UpgradeButton->SetIsEnabled(bButtonEnabled);
		break;

	case EItemStatType::AttackSpeed:
		if (AtkSpd_LevelText) AtkSpd_LevelText->SetText(LevelText);
		if (AtkSpd_CostText) AtkSpd_CostText->SetText(CostText);
		if (AtkSpd_UpgradeButton) AtkSpd_UpgradeButton->SetIsEnabled(bButtonEnabled);
		break;

	case EItemStatType::CritDamage:
		if (CritDmg_LevelText) CritDmg_LevelText->SetText(LevelText);
		if (CritDmg_CostText) CritDmg_CostText->SetText(CostText);
		if (CritDmg_UpgradeButton) CritDmg_UpgradeButton->SetIsEnabled(bButtonEnabled);
		break;

	case EItemStatType::ArmorReduction:
		if (ArmorReduction_LevelText) ArmorReduction_LevelText->SetText(LevelText);
		if (ArmorReduction_CostText) ArmorReduction_CostText->SetText(CostText);
		if (ArmorReduction_ChanceText) ArmorReduction_ChanceText->SetText(ChanceText);
		if (ArmorReduction_UpgradeButton) ArmorReduction_UpgradeButton->SetIsEnabled(bButtonEnabled);
		break;

	case EItemStatType::SkillActivationChance:
		if (SkillChance_LevelText) SkillChance_LevelText->SetText(LevelText);
		if (SkillChance_CostText) SkillChance_CostText->SetText(CostText);
		if (SkillChance_ChanceText) SkillChance_ChanceText->SetText(ChanceText);
		if (SkillChance_UpgradeButton) SkillChance_UpgradeButton->SetIsEnabled(bButtonEnabled);
		break;

	default: break; // ��ȭ �Ұ� ������ UI ������Ʈ �� ��
	}
}


// --- ���� ���� �� ���� �ڵ鷯 ���� ---
// ASC�� Attribute ���� ����� �� ȣ��Ǿ� ValueText ������Ʈ
void UStatUpgradeWidget::HandleAttackDamageChanged(const FOnAttributeChangeData& Data)
{
	if (AtkDmg_ValueText) AtkDmg_ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue))); // ������ �ݿø� ǥ��
}

void UStatUpgradeWidget::HandleAttackSpeedChanged(const FOnAttributeChangeData& Data)
{
	// ���� �ӵ��� �Ҽ��� �ۼ�Ʈ�� ǥ�� (��: 0.15 -> 15.0%)
	if (AtkSpd_ValueText) AtkSpd_ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f%%"), Data.NewValue * 100.0f)));
}

void UStatUpgradeWidget::HandleCritDamageChanged(const FOnAttributeChangeData& Data)
{
	if (CritDmg_ValueText) CritDmg_ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue)));
}

void UStatUpgradeWidget::HandleArmorReductionChanged(const FOnAttributeChangeData& Data)
{
	if (ArmorReduction_ValueText) ArmorReduction_ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue)));
}

void UStatUpgradeWidget::HandleSkillChanceChanged(const FOnAttributeChangeData& Data)
{
	// ��ų �ߵ� Ȯ���� �Ҽ��� �ۼ�Ʈ�� ǥ��
	if (SkillChance_ValueText) SkillChance_ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f%%"), Data.NewValue * 100.0f)));
}
// (�ʿ�� �ٸ� ���� �� �ڵ鷯�� ���� - ��� ��ȭ �Ұ� ���� ���� ����)


// --- ��ư Ŭ�� �ڵ鷯 ���� (PlayerState�� ���� �Լ� ȣ��) ---
void UStatUpgradeWidget::HandleUpgradeAtkDmg() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::AttackDamage); }
void UStatUpgradeWidget::HandleUpgradeAtkSpd() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::AttackSpeed); }
void UStatUpgradeWidget::HandleUpgradeCritDmg() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::CritDamage); }
void UStatUpgradeWidget::HandleUpgradeArmorReduction() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::ArmorReduction); }
void UStatUpgradeWidget::HandleUpgradeSkillChance() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::SkillActivationChance); }