#include "StatUpgradeWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MyPlayerState.h"
#include "MyAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "RamdomItemDefenseCharacter.h" 
#include "GameplayEffectTypes.h"
#include "Engine/Engine.h"
#include "RamdomItemDefense.h"
#include "MyGameState.h" 

void UStatUpgradeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AtkDmg_UpgradeButton) AtkDmg_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeAtkDmg);
	if (AtkSpd_UpgradeButton) AtkSpd_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeAtkSpd);
	if (CritDmg_UpgradeButton) CritDmg_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeCritDmg);
	if (ArmorReduction_UpgradeButton) ArmorReduction_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeArmorReduction);
	if (SkillChance_UpgradeButton) SkillChance_UpgradeButton->OnClicked.AddDynamic(this, &UStatUpgradeWidget::HandleUpgradeSkillChance);
}

void UStatUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// [수정] 바인딩 함수 호출
	BindPlayerState();

	// 캐릭터의 ASC 참조 가져오기 (이 부분도 안전하게 재시도 로직에 넣거나, ASC는 보통 BeginPlay 시점에 이미 있으므로 유지)
	ARamdomItemDefenseCharacter* OwningCharacter = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>();
	if (OwningCharacter)
	{
		OwningASC = OwningCharacter->GetAbilitySystemComponent();
		if (OwningASC.IsValid())
		{
			// (기존 ASC 델리게이트 바인딩 로직 그대로 유지)
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackDamageAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleAttackDamageChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleAttackSpeedChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritDamageAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleCritDamageChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetArmorReductionAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleArmorReductionChanged);
			OwningASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetSkillActivationChanceAttribute()).AddUObject(this, &UStatUpgradeWidget::HandleSkillChanceChanged);

			// 초기값 업데이트
			FOnAttributeChangeData DummyData;
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetAttackDamageAttribute()); HandleAttackDamageChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetAttackSpeedAttribute()); HandleAttackSpeedChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetCritDamageAttribute()); HandleCritDamageChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetArmorReductionAttribute()); HandleArmorReductionChanged(DummyData);
			DummyData.NewValue = OwningASC->GetNumericAttribute(UMyAttributeSet::GetSkillActivationChanceAttribute()); HandleSkillChanceChanged(DummyData);
		}
	}
}

void UStatUpgradeWidget::BindPlayerState()
{
	// 이미 바인딩 되었다면 중복 실행 방지 (선택 사항)
	if (MyPlayerState) return;

	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();

	if (MyPlayerState)
	{
		// 델리게이트 중복 바인딩 방지 체크
		if (!MyPlayerState->OnGoldChangedDelegate.IsAlreadyBound(this, &UStatUpgradeWidget::HandleGoldChanged))
		{
			MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleGoldChanged);
		}
		if (!MyPlayerState->OnStatLevelChangedDelegate.IsAlreadyBound(this, &UStatUpgradeWidget::HandleStatLevelChanged))
		{
			MyPlayerState->OnStatLevelChangedDelegate.AddDynamic(this, &UStatUpgradeWidget::HandleStatLevelChanged);
		}

		// 초기 UI 업데이트
		HandleGoldChanged(MyPlayerState->GetGold());
		UpdateAllStatLines();

		UE_LOG(LogRamdomItemDefense, Log, TEXT("StatUpgradeWidget: PlayerState Bound Successfully!"));
	}
	else
	{
		// [핵심] 실패 시 다음 틱에 자기 자신을 다시 호출 (무한 재시도)
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UStatUpgradeWidget::BindPlayerState);
	}
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

	AMyGameState* GameState = GetWorld()->GetGameState<AMyGameState>();
	if (!GameState) return; // 아직 GameState 동기화 전이면 중단

	int32 CurrentLevel = MyPlayerState->GetStatLevel(StatType);
	int32 CurrentGold = MyPlayerState->GetGold();

	// --- 강화 규칙 (PlayerState와 동일하게 유지) ---
	const bool bIsBasicStat = (StatType == EItemStatType::AttackDamage || StatType == EItemStatType::AttackSpeed || StatType == EItemStatType::CritDamage);
	const bool bIsSpecialStat = (StatType == EItemStatType::ArmorReduction || StatType == EItemStatType::SkillActivationChance);

	// --- [코드 수정] 매직 넘버를 매크로로 대체 ---
	int32 MaxLevel = bIsBasicStat ? GameState->MaxNormalStatLevel : GameState->MaxSpecialStatLevel;
	int32 BaseCost = GameState->BaseLevelUpCost;
	int32 CostIncreaseFactor = GameState->IncreasingCostPerLevel;
	// ---------------------------------------------

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
		// --- [수정] 매크로 대신 GameState 배열 사용 ---
		if (GameState->SpecialStatUpgradeChances.IsValidIndex(CurrentLevel))
		{
			SuccessChance = GameState->SpecialStatUpgradeChances[CurrentLevel];
		}
		else
		{
			SuccessChance = 0.0f; // 범위 밖(최대 레벨 등)
		}
		// ---------------------------------------------
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

	if (CritDmg_ValueText) CritDmg_ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f%%"), Data.NewValue * 100.0f)));
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


/** 모든 업그레이드 버튼을 일시적으로 비활성화합니다. (RPC 중복 전송 방지) */
void UStatUpgradeWidget::DisableAllUpgradeButtons()
{
	if (AtkDmg_UpgradeButton) AtkDmg_UpgradeButton->SetIsEnabled(false);
	if (AtkSpd_UpgradeButton) AtkSpd_UpgradeButton->SetIsEnabled(false);
	if (CritDmg_UpgradeButton) CritDmg_UpgradeButton->SetIsEnabled(false);
	if (ArmorReduction_UpgradeButton) ArmorReduction_UpgradeButton->SetIsEnabled(false);
	if (SkillChance_UpgradeButton) SkillChance_UpgradeButton->SetIsEnabled(false);
}


// --- 버튼 클릭 핸들러 구현 (PlayerState의 서버 함수 호출) ---
// (RPC 중복 전송 방지 로직 포함)
void UStatUpgradeWidget::HandleUpgradeAtkDmg()
{
	// 버튼이 활성화 상태일 때만 RPC 전송
	if (MyPlayerState && AtkDmg_UpgradeButton && AtkDmg_UpgradeButton->GetIsEnabled())
	{
		DisableAllUpgradeButtons(); // (추가) 즉시 모든 버튼 비활성화
		MyPlayerState->Server_RequestStatUpgrade(EItemStatType::AttackDamage);
	}
}
void UStatUpgradeWidget::HandleUpgradeAtkSpd()
{
	if (MyPlayerState && AtkSpd_UpgradeButton && AtkSpd_UpgradeButton->GetIsEnabled())
	{
		DisableAllUpgradeButtons(); // (추가) 즉시 모든 버튼 비활성화
		MyPlayerState->Server_RequestStatUpgrade(EItemStatType::AttackSpeed);
	}
}
void UStatUpgradeWidget::HandleUpgradeCritDmg()
{
	if (MyPlayerState && CritDmg_UpgradeButton && CritDmg_UpgradeButton->GetIsEnabled())
	{
		DisableAllUpgradeButtons(); // (추가) 즉시 모든 버튼 비활성화
		MyPlayerState->Server_RequestStatUpgrade(EItemStatType::CritDamage);
	}
}
void UStatUpgradeWidget::HandleUpgradeArmorReduction()
{
	if (MyPlayerState && ArmorReduction_UpgradeButton && ArmorReduction_UpgradeButton->GetIsEnabled())
	{
		DisableAllUpgradeButtons(); // (추가) 즉시 모든 버튼 비활성화
		MyPlayerState->Server_RequestStatUpgrade(EItemStatType::ArmorReduction);
	}
}
void UStatUpgradeWidget::HandleUpgradeSkillChance()
{
	if (MyPlayerState && SkillChance_UpgradeButton && SkillChance_UpgradeButton->GetIsEnabled())
	{
		DisableAllUpgradeButtons(); // (추가) 즉시 모든 버튼 비활성화
		MyPlayerState->Server_RequestStatUpgrade(EItemStatType::SkillActivationChance);
	}
}