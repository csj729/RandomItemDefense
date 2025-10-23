#include "StatUpgradeWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MyPlayerState.h"
#include "MyAttributeSet.h" // Attribute 정의 포함
#include "AbilitySystemComponent.h" // ASC 사용
#include "RamdomItemDefenseCharacter.h" // 캐릭터 클래스 (ASC 가져오기 위함)
#include "GameplayEffectTypes.h"
#include "Engine/Engine.h" // GEngine 디버그 메시지 (선택 사항)

void UStatUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// PlayerState 참조 가져오기 및 델리게이트 바인딩
	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	if (MyPlayerState)
	{
		MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleGoldChanged);
		MyPlayerState->OnStatLevelChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleStatLevelChanged);

		// 초기 UI 업데이트
		HandleGoldChanged(MyPlayerState->GetGold());
		UpdateAllStatLines(); // 레벨, 비용, 버튼 상태 초기화
	}
	else
	{
		// PlayerState가 늦게 로드될 경우 다음 틱에 재시도
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
			if (MyPlayerState)
			{
				MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleGoldChanged);
				MyPlayerState->OnStatLevelChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleStatLevelChanged);
				HandleGoldChanged(MyPlayerState->GetGold());
				UpdateAllStatLines();
				// ASC 바인딩도 여기서 다시 시도해야 할 수 있음
				// ... (아래 ASC 바인딩 로직 복사) ...
			}
			});
	}

	// 캐릭터의 ASC 참조 가져오기 및 스탯 변경 델리게이트 바인딩
	ARamdomItemDefenseCharacter* OwningCharacter = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>();
	if (OwningCharacter)
	{
		OwningASC = OwningCharacter->GetAbilitySystemComponent();
		if (OwningASC.IsValid())
		{
			// 각 Attribute 변경 시 해당 핸들러 함수를 호출하도록 연결
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackDamageAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleAttackDamageChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleAttackSpeedChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritDamageAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleCritDamageChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetArmorReductionAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleArmorReductionChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetSkillActivationChanceAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleSkillChanceChanged);
			// (골드 강화 불가 스탯 값 표시가 필요하다면 여기서 바인딩 추가)
			// OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritChanceAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleCritChanceChanged); // 예시

			// 초기 스탯 값 UI 업데이트
			// 각 핸들러를 직접 호출하여 초기 값을 설정합니다.
			FOnAttributeChangeData DummyData; // 임시 데이터 구조체
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetAttackDamageAttribute()); HandleAttackDamageChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetAttackSpeedAttribute()); HandleAttackSpeedChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetCritDamageAttribute()); HandleCritDamageChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetArmorReductionAttribute()); HandleArmorReductionChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetSkillActivationChanceAttribute()); HandleSkillChanceChanged(DummyData);
			// (다른 스탯 초기값 설정도 필요시 추가)
		}
	}


	// 버튼 클릭 이벤트 바인딩
	if (AtkDmg_UpgradeButton) AtkDmg_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeAtkDmg);
	if (AtkSpd_UpgradeButton) AtkSpd_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeAtkSpd);
	if (CritDmg_UpgradeButton) CritDmg_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeCritDmg);
	if (ArmorReduction_UpgradeButton) ArmorReduction_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeArmorReduction);
	if (SkillChance_UpgradeButton) SkillChance_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeSkillChance);
}

// 골드 변경 시 호출
void UStatUpgradeWidget::HandleGoldChanged(int32 NewGold)
{
	if (GoldText)
	{
		// 골드 텍스트 업데이트 (예: FText::AsCurrency(NewGold, TEXT("G")))
		GoldText->SetText(FText::FromString(FString::Printf(TEXT("남은 골드 : %d G"), NewGold)));
	}
	// 골드가 변경되면 모든 버튼의 활성화 상태를 다시 계산
	UpdateAllStatLines();
}

// 스탯 레벨 변경 시 호출 (PlayerState RepNotify -> Delegate)
void UStatUpgradeWidget::HandleStatLevelChanged(EItemStatType StatType, int32 NewLevel)
{
	// 해당 스탯 라인의 레벨, 비용, 확률, 버튼 상태 업데이트
	UpdateStatLineUI(StatType);
}

// 모든 스탯 라인 UI 업데이트 (초기화 및 골드 변경 시)
void UStatUpgradeWidget::UpdateAllStatLines()
{
	// 모든 강화 가능한 스탯에 대해 UI 업데이트 호출
	UpdateStatLineUI(EItemStatType::AttackDamage);
	UpdateStatLineUI(EItemStatType::AttackSpeed);
	UpdateStatLineUI(EItemStatType::CritDamage);
	UpdateStatLineUI(EItemStatType::ArmorReduction);
	UpdateStatLineUI(EItemStatType::SkillActivationChance);
	// (골드 강화 불가 스탯은 호출할 필요 없음)
}

/** 특정 스탯 라인 UI 업데이트 (레벨, 비용, 확률, 버튼 상태) */
void UStatUpgradeWidget::UpdateStatLineUI(EItemStatType StatType)
{
	if (!MyPlayerState) return;

	int32 CurrentLevel = MyPlayerState->GetStatLevel(StatType);
	int32 CurrentGold = MyPlayerState->GetGold();

	// --- 강화 규칙 (PlayerState와 동일하게 유지) ---
	const bool bIsBasicStat = (StatType == EItemStatType::AttackDamage || StatType == EItemStatType::AttackSpeed || StatType == EItemStatType::CritDamage);
	const bool bIsSpecialStat = (StatType == EItemStatType::ArmorReduction || StatType == EItemStatType::SkillActivationChance);
	int32 MaxLevel = bIsBasicStat ? 999 : 3;
	int32 BaseCost = 100;
	int32 CostIncreaseFactor = 50;
	float SuccessChance = 1.0f;
	// ---------------------------------------------

	// 레벨 제한 확인
	bool bMaxLevelReached = (CurrentLevel >= MaxLevel);

	// 비용 계산
	int32 UpgradeCost = BaseCost + (CurrentLevel * CostIncreaseFactor);
	bool bCanAfford = (CurrentGold >= UpgradeCost);

	// 특수 스탯 성공 확률 계산 (표시용)
	if (bIsSpecialStat)
	{
		switch (CurrentLevel)
		{
		case 0: SuccessChance = 0.5f; break; // 50%
		case 1: SuccessChance = 0.4f; break; // 40%
		case 2: SuccessChance = 0.3f; break; // 30%
		default: SuccessChance = 0.0f; break; // 최대 레벨 도달
		}
	}

	// UI 요소 업데이트 (Switch 사용)
	FText LevelText = FText::Format(NSLOCTEXT("StatUpgradeWidget", "LevelFormat", "Lv.{0}"), FText::AsNumber(CurrentLevel));
	FText CostText = FText::Format(NSLOCTEXT("StatUpgradeWidget", "CostFormat", "{0}G"), FText::AsNumber(UpgradeCost));
	FText ChanceText = FText::Format(NSLOCTEXT("StatUpgradeWidget", "ChanceFormat", "{0}"), FText::AsPercent(SuccessChance));
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

	default: break; // 강화 불가 스탯은 UI 업데이트 안 함
	}
}


// --- 개별 스탯 값 변경 핸들러 구현 ---
// ASC의 Attribute 값이 변경될 때 호출되어 ValueText 업데이트
void UStatUpgradeWidget::HandleAttackDamageChanged(const FOnAttributeChangeData& Data)
{
	if (AtkDmg_ValueText) AtkDmg_ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue))); // 정수로 반올림 표시
}

void UStatUpgradeWidget::HandleAttackSpeedChanged(const FOnAttributeChangeData& Data)
{
	// 공격 속도는 소수점 퍼센트로 표시 (예: 0.15 -> 15.0%)
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
	// 스킬 발동 확률도 소수점 퍼센트로 표시
	if (SkillChance_ValueText) SkillChance_ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f%%"), Data.NewValue * 100.0f)));
}
// (필요시 다른 스탯 값 핸들러도 구현 - 골드 강화 불가 스탯 포함 가능)


// --- 버튼 클릭 핸들러 구현 (PlayerState의 서버 함수 호출) ---
void UStatUpgradeWidget::HandleUpgradeAtkDmg() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::AttackDamage); }
void UStatUpgradeWidget::HandleUpgradeAtkSpd() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::AttackSpeed); }
void UStatUpgradeWidget::HandleUpgradeCritDmg() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::CritDamage); }
void UStatUpgradeWidget::HandleUpgradeArmorReduction() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::ArmorReduction); }
void UStatUpgradeWidget::HandleUpgradeSkillChance() { if (MyPlayerState) MyPlayerState->Server_RequestStatUpgrade(EItemStatType::SkillActivationChance); }