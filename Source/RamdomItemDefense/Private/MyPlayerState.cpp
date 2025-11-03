// Source/RamdomItemDefense/Private/MyPlayerState.cpp (수정)

#include "MyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "MonsterSpawner.h"
#include "RamdomItemDefenseCharacter.h"
#include "InventoryComponent.h"
#include "RamdomItemDefense.h" // RID_LOG 매크로용
#include "MyGameState.h"       // GetCurrentWave() 사용
#include "Engine/World.h"      // GetWorld() 사용

AMyPlayerState::AMyPlayerState()
{
	Gold = 0;
	ChoiceCount = 0;
	CommonItemChoiceCount = 0; // [코드 추가] 초기화
	UltimateCharge = 0;

	// 모든 강화 가능한 스탯의 초기 레벨을 0으로 설정
	AttackDamageLevel = 0;
	AttackSpeedLevel = 0;
	CritDamageLevel = 0;
	ArmorReductionLevel = 0;
	SkillActivationChanceLevel = 0;
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyPlayerState, Gold);
	DOREPLIFETIME(AMyPlayerState, ChoiceCount);
	DOREPLIFETIME(AMyPlayerState, MySpawner);

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	DOREPLIFETIME(AMyPlayerState, CommonItemChoiceCount); // 복제 등록
	DOREPLIFETIME(AMyPlayerState, UltimateCharge); // 복제 등록
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

	DOREPLIFETIME(AMyPlayerState, AttackDamageLevel);
	DOREPLIFETIME(AMyPlayerState, AttackSpeedLevel);
	DOREPLIFETIME(AMyPlayerState, CritDamageLevel);
	DOREPLIFETIME(AMyPlayerState, ArmorReductionLevel);
	DOREPLIFETIME(AMyPlayerState, SkillActivationChanceLevel);
}

// --- 골드 관련 (수정됨) ---
void AMyPlayerState::AddGold(int32 Amount)
{
	if (HasAuthority())
	{
		Gold += Amount;
		OnRep_Gold(); // 서버에서도 델리게이트 호출
	}
}

// --- 골드 소모 함수 ---
bool AMyPlayerState::SpendGold(int32 Amount)
{
	if (!HasAuthority()) return false; // 서버에서만 실행
	if (Amount <= 0) return true; // 0 이하 소모는 항상 성공

	if (Gold >= Amount)
	{
		Gold -= Amount;
		OnRep_Gold(); // 서버 UI 즉시 업데이트 및 클라이언트 복제 트리거
		return true; // 소모 성공
	}
	return false; // 골드 부족
}

void AMyPlayerState::OnRep_Gold()
{
	// 골드 변경 델리게이트를 방송합니다.
	OnGoldChangedDelegate.Broadcast(Gold);
}

void AMyPlayerState::OnRep_MySpawner()
{
	// 스포너가 유효하게 할당되었음을 UI(MainHUD)에 알립니다.
	OnSpawnerAssignedDelegate.Broadcast(0); // 0은 의미 없는 값
}

// --- [코드 수정] 라운드 선택 (뽑기/도박) 관련 ---

/**
 * @brief (서버 전용) 라운드 선택 횟수(뽑기/도박)를 추가합니다.
 */
void AMyPlayerState::AddChoiceCount(int32 Count)
{
	if (HasAuthority())
	{
		ChoiceCount += Count;
		OnRep_ChoiceCount(); // 서버에서도 델리게이트 호출
	}
}

/**
 * @brief 클라이언트에서 ChoiceCount가 복제되었을 때 호출됩니다.
 */
void AMyPlayerState::OnRep_ChoiceCount()
{
	// 선택 횟수 변경 델리게이트를 방송합니다.
	OnChoiceCountChangedDelegate.Broadcast(ChoiceCount);
}

/**
 * @brief (UI에서 호출) 라운드 선택(아이템/골드)을 사용합니다.
 */
void AMyPlayerState::Server_UseRoundChoice_Implementation(bool bChoseItemGacha)
{
	// (서버에서 실행됨)
	if (ChoiceCount <= 0)
	{
		return; // 선택 횟수가 없으면 무시
	}

	ChoiceCount--;
	OnRep_ChoiceCount(); // 서버에서 즉시 델리게이트 호출

	if (bChoseItemGacha)
	{
		// 이 PlayerState가 소유한 폰(캐릭터)을 가져옵니다.
		ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
		if (Character && Character->GetInventoryComponent())
		{
			// 캐릭터의 인벤토리 컴포넌트에게 무작위 아이템을 추가하라고 명령합니다.
			// [주의] AddRandomItem은 이제 '흔함' 등급만 뽑습니다. 기획 의도 확인 필요.
			Character->GetInventoryComponent()->AddRandomItem();
		}

		RID_LOG(FColor::Cyan, TEXT("Player chose: Item Gacha (Common)"));
	}
	else
	{
		// --- [ ★★★ 골드 도박 로직 수정 ★★★ ] ---

		// 1. GameState에서 현재 웨이브 가져오기
		AMyGameState* MyGameState = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
		int32 CurrentWave = 1; // GameState가 없거나 웨이브가 0이면 1로 간주
		if (MyGameState && MyGameState->GetCurrentWave() > 0)
		{
			CurrentWave = MyGameState->GetCurrentWave();
		}

		// 2. 웨이브 기반으로 골드 계산 (기본값: 50 * Wave, 범위: ± 30)
		const int32 BaseAmount = 50 * CurrentWave;
		const int32 RandomBonus = FMath::RandRange(-30, 30); // TodoList "± 일정값"
		const int32 GambleAmount = FMath::Max(1, BaseAmount + RandomBonus); // 최소 1골드

		AddGold(GambleAmount);
		RID_LOG(FColor::Cyan, TEXT("Player chose: Gold Gamble (Wave: %d, Base: %d, Final: +%d)"), CurrentWave, BaseAmount, GambleAmount);
		// --- [ ★★★ 로직 수정 끝 ★★★ ] ---
	}
}


// --- [ ★★★ 코드 추가 (흔함 아이템 선택) ★★★ ] ---

/**
 * @brief (서버 전용) '흔함 아이템 선택권' 횟수를 추가합니다.
 */
void AMyPlayerState::AddCommonItemChoice(int32 Count)
{
	if (HasAuthority())
	{
		CommonItemChoiceCount += Count;
		OnRep_CommonItemChoiceCount(); // 서버에서도 델리게이트 호출
	}
}

/**
 * @brief (새 UI에서 호출) '흔함 아이템 선택권'을 사용하고 아이템을 획득합니다.
 */
void AMyPlayerState::Server_UseCommonItemChoice_Implementation(FName ChosenItemID)
{
	// (서버에서 실행됨)
	if (CommonItemChoiceCount <= 0)
	{
		return; // 선택 횟수가 없으면 무시
	}

	// 데이터 유효성 검사 (선택한 아이템이 실제 '흔함' 등급인지 등)
	ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
	if (!Character || !Character->GetInventoryComponent()) return;

	bool bSuccess = false;
	FItemData ItemData = Character->GetInventoryComponent()->GetItemData(ChosenItemID, bSuccess);

	// [검증] 아이템 ID가 유효하고, '흔함' 등급이어야만 선택권 사용 가능
	if (bSuccess && ItemData.Grade == EItemGrade::Common)
	{
		CommonItemChoiceCount--;
		OnRep_CommonItemChoiceCount(); // 서버에서 즉시 델리게이트 호출

		// 캐릭터의 인벤토리 컴포넌트에게 '선택한' 아이템을 추가하라고 명령합니다.
		Character->GetInventoryComponent()->AddItem(ChosenItemID);

		RID_LOG(FColor::Green, TEXT("Player used Common Item Choice: Added %s"), *ChosenItemID.ToString());
	}
	else
	{
		RID_LOG(FColor::Red, TEXT("Server_UseCommonItemChoice: Invalid ItemID %s or not Common grade."), *ChosenItemID.ToString());
	}
}

/**
 * @brief 클라이언트에서 CommonItemChoiceCount가 복제되었을 때 호출됩니다.
 */
void AMyPlayerState::OnRep_CommonItemChoiceCount()
{
	// '흔함 아이템 선택권' 횟수 변경 델리게이트를 방송합니다.
	OnCommonItemChoiceCountChangedDelegate.Broadcast(CommonItemChoiceCount);
}
// --- [ ★★★ 코드 추가 끝 ★★★ ] ---


/** 클라이언트에서 StatLevels 복제 시 호출 */
// 각 OnRep 함수는 해당 스탯 타입과 새 레벨로 델리게이트를 호출합니다.
void AMyPlayerState::OnRep_AttackDamageLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::AttackDamage, AttackDamageLevel); }
void AMyPlayerState::OnRep_AttackSpeedLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::AttackSpeed, AttackSpeedLevel); }
void AMyPlayerState::OnRep_CritDamageLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::CritDamage, CritDamageLevel); }
void AMyPlayerState::OnRep_ArmorReductionLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::ArmorReduction, ArmorReductionLevel); }
void AMyPlayerState::OnRep_SkillActivationChanceLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::SkillActivationChance, SkillActivationChanceLevel); }

/** 특정 스탯 레벨 반환 */
int32 AMyPlayerState::GetStatLevel(EItemStatType StatType) const
{
	// --- [코드 수정] switch 문 사용 ---
	switch (StatType)
	{
	case EItemStatType::AttackDamage: return AttackDamageLevel;
	case EItemStatType::AttackSpeed: return AttackSpeedLevel;
	case EItemStatType::CritDamage: return CritDamageLevel;
	case EItemStatType::ArmorReduction: return ArmorReductionLevel;
	case EItemStatType::SkillActivationChance: return SkillActivationChanceLevel;
	default: return 0;
	}
	// ----------------------------------
}

/** UI에서 호출하는 서버 RPC */
void AMyPlayerState::Server_RequestStatUpgrade_Implementation(EItemStatType StatToUpgrade)
{
	// 서버에서 실제 강화 시도
	TryUpgradeStat(StatToUpgrade);
}

/** (서버 전용) 실제 강화 로직 */
/** (서버 전용) 실제 강화 로직 */
bool AMyPlayerState::TryUpgradeStat(EItemStatType StatToUpgrade)
{
	if (!HasAuthority()) return false;

	// 강화 가능 스탯 확인 (변경 없음)
	const bool bIsGoldUpgradableBasicStat = (StatToUpgrade == EItemStatType::AttackDamage ||
		StatToUpgrade == EItemStatType::AttackSpeed ||
		StatToUpgrade == EItemStatType::CritDamage);

	const bool bIsGoldUpgradableSpecialStat = (StatToUpgrade == EItemStatType::ArmorReduction ||
		StatToUpgrade == EItemStatType::SkillActivationChance);

	if (!bIsGoldUpgradableBasicStat && !bIsGoldUpgradableSpecialStat)
	{
		RID_LOG(FColor::Red, TEXT("TryUpgradeStat: %s cannot be upgraded with gold."), *UEnum::GetValueAsString(StatToUpgrade));
		return false;
	}

	int32 CurrentLevel = GetStatLevel(StatToUpgrade);

	// --- 강화 규칙 (변경 없음) ---
	int32 MaxLevel = bIsGoldUpgradableBasicStat ? MAX_NORMAL_STAT_LEVEL : MAX_SPECIAL_STAT_LEVEL; // 특수 스탯 최대 3레벨
	int32 BaseCost = BASE_LEVELUP_COST;
	int32 CostIncreaseFactor = INCREASING_COST_PER_LEVEL;
	// ---------------------------

	// 레벨 제한 확인 (변경 없음)
	if (CurrentLevel >= MaxLevel)
	{
		RID_LOG(FColor::Yellow, TEXT("TryUpgradeStat: Max level reached"));
		return false;
	}

	// 비용 계산 (변경 없음)
	int32 UpgradeCost = BaseCost + (CurrentLevel * CostIncreaseFactor);

	// 골드 확인 및 소모 (변경 없음)
	if (!SpendGold(UpgradeCost))
	{
		RID_LOG(FColor::Yellow, TEXT("TryUpgradeStat: Not enough gold"));
		return false;
	}

	// --- [코드 수정] 성공 확률 계산 (새로운 규칙 적용) ---
	bool bUpgradeSuccess = true; // 기본 스탯은 항상 성공
	float SuccessChance = 1.0f; // 성공 확률 기록용 변수

	if (bIsGoldUpgradableSpecialStat) // 특수 스탯일 경우 확률 적용
	{
		// CurrentLevel 기준으로 다음 레벨로 갈 확률 설정
		switch (CurrentLevel)
		{
			// --- [코드 수정] 매직 넘버를 매크로로 대체 ---
		case 0: // 0 -> 1 레벨 강화 시도
			SuccessChance = SPECIAL_STAT_UPGRADE_CHANCE_LVL0; // 50%
			break;
		case 1: // 1 -> 2 레벨 강화 시도
			SuccessChance = SPECIAL_STAT_UPGRADE_CHANCE_LVL1; // 40%
			break;
		case 2: // 2 -> 3 레벨 강화 시도
			SuccessChance = SPECIAL_STAT_UPGRADE_CHANCE_LVL2; // 30%
			break;
			// ------------------------------------------
		default: // 이미 최대 레벨이거나 예외 상황 (이론상 여기에 도달하면 안 됨)
			SuccessChance = 0.0f;
			break;
		}
		// 0.0 ~ 1.0 사이 난수 생성하여 성공 여부 판정
		bUpgradeSuccess = (FMath::FRand() < SuccessChance);
	}
	// ----------------------------------------------------

	// 강화 성공 시 (변경 없음)
	if (bUpgradeSuccess)
	{
		// 1. 레벨 증가 및 변수 업데이트 (switch 사용)
		int32 NewLevel = CurrentLevel + 1;
		bool bLevelUpdated = false;
		switch (StatToUpgrade)
		{
		case EItemStatType::AttackDamage: AttackDamageLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::AttackSpeed: AttackSpeedLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::CritDamage: CritDamageLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::ArmorReduction: ArmorReductionLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::SkillActivationChance: SkillActivationChanceLevel = NewLevel; bLevelUpdated = true; break;
		}

		// 레벨 업데이트 성공 시 서버 UI 즉시 업데이트 및 클라이언트 복제 트리거
		if (bLevelUpdated)
		{
			// 변경된 변수의 OnRep 함수 직접 호출 (변경 없음)
			switch (StatToUpgrade)
			{
			case EItemStatType::AttackDamage: OnRep_AttackDamageLevel(); break;
			case EItemStatType::AttackSpeed: OnRep_AttackSpeedLevel(); break;
			case EItemStatType::CritDamage: OnRep_CritDamageLevel(); break;
			case EItemStatType::ArmorReduction: OnRep_ArmorReductionLevel(); break;
			case EItemStatType::SkillActivationChance: OnRep_SkillActivationChanceLevel(); break;
			}

			// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
			FString StatName = UEnum::GetValueAsString(StatToUpgrade);
			// 특수 스탯 성공 시 확률도 함께 표시 (선택 사항)
			FString ChanceString = bIsGoldUpgradableSpecialStat ? FString::Printf(TEXT(" (Chance: %.0f%%)"), SuccessChance * 100) : TEXT("");
			RID_LOG(FColor::Green, TEXT("Upgrade Success: %s to Level %d (Cost: %d)%s"), *StatName, NewLevel, UpgradeCost, *ChanceString);
			// -----------------------------------------
		}

		// 2. 실제 스탯 적용 요청 (캐릭터 ASC에) (변경 없음)
		ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
		if (Character)
		{
			Character->ApplyStatUpgrade(StatToUpgrade, NewLevel);
		}
		return true;
	}
	else // 강화 실패 시 (특수 스탯 - 로그 변경 없음)
	{
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		RID_LOG(FColor::Orange, TEXT("Upgrade Failed: %s (Level %d -> %d, Cost: %d, Chance: %.0f%%)"),
			*UEnum::GetValueAsString(StatToUpgrade), CurrentLevel, CurrentLevel + 1, UpgradeCost, SuccessChance * 100);
		// -----------------------------------------
		return false;
	}
}

/**
 * @brief (서버 전용) 궁극기 스택을 추가합니다.
 */
void AMyPlayerState::AddUltimateCharge(int32 Amount)
{
	if (HasAuthority() && UltimateCharge < MAX_ULTIMATE_CHARGE)
	{
		// 최대치를 넘지 않도록 FMath::Min 사용
		UltimateCharge = FMath::Min(UltimateCharge + Amount, MAX_ULTIMATE_CHARGE);

		// 서버에서도 델리게이트 즉시 호출 (서버 UI 반영용)
		OnRep_UltimateCharge();
	}
}

/**
 * @brief (서버 전용) 궁극기 스택을 0으로 리셋합니다.
 */
void AMyPlayerState::ResetUltimateCharge()
{
	if (HasAuthority())
	{
		UltimateCharge = 0;
		OnRep_UltimateCharge();
	}
}

/**
 * @brief 클라이언트에서 UltimateCharge가 복제되었을 때 호출됩니다.
 */
void AMyPlayerState::OnRep_UltimateCharge()
{
	// UI(WBP_MainHUD)에 스택이 변경되었음을 알립니다.
	OnUltimateChargeChangedDelegate.Broadcast(UltimateCharge);
}

int32 AMyPlayerState::GetMaxUltimateCharge() const
{
	return MAX_ULTIMATE_CHARGE;
}