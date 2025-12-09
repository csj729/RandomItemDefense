// Source/RamdomItemDefense/Private/MyPlayerState.cpp (수정)

#include "MyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "MonsterSpawner.h"
#include "RamdomItemDefenseCharacter.h"
#include "RamdomItemDefensePlayerController.h"
#include "AbilitySystemComponent.h"
#include "InventoryComponent.h"
#include "RamdomItemDefense.h"
#include "MyGameState.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "RamdomItemDefenseGameMode.h"

DEFINE_LOG_CATEGORY(LogRID_PlayerState);

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

	// --- 버튼 액션 관련 변수 전체 초기화 ---
	ButtonActionLevel = 0;
	bIsButtonActionSequenceFinishedThisStage = false;
	bIsWaitingForButtonActionInput = false;
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

	// [ ★★★ 버튼 액션 변수 복제 등록 ★★★ ]
	DOREPLIFETIME(AMyPlayerState, ButtonActionLevel);
	DOREPLIFETIME(AMyPlayerState, bIsButtonActionSequenceFinishedThisStage);

	DOREPLIFETIME(AMyPlayerState, SelectedCharacterClass);
	DOREPLIFETIME(AMyPlayerState, bIsReady);
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

void AMyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	// 1. 부모 클래스의 로직 실행 (필수: 점수, Ping 등 기본 정보 복사)
	Super::CopyProperties(PlayerState);

	// 2. 인자로 들어온 PlayerState를 내 클래스(AMyPlayerState)로 캐스팅
	AMyPlayerState* MyNewPS = Cast<AMyPlayerState>(PlayerState);
	if (MyNewPS)
	{
		// 3. [핵심] '나(this, 옛날 PS)'의 데이터를 '새 PS(MyNewPS)'에 복사
		MyNewPS->SelectedCharacterClass = this->SelectedCharacterClass;

		// [로그 확인] 복사가 잘 되었는지 확인 (Cyan 색상)
		RID_LOG(FColor::Cyan, TEXT("CopyProperties: Transferred Class '%s' to New Map for Player '%s'"),
			*GetNameSafe(this->SelectedCharacterClass), *GetPlayerName());
	}
	else
	{
		RID_LOG(FColor::Cyan, TEXT("CopyProperties: PS is Null"));
	}
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

		LOG_PLAYERSTATE(FColor::Cyan, TEXT("Player chose: Item Gacha (Common)"));
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
		LOG_PLAYERSTATE(FColor::Cyan, TEXT("Player chose: Gold Gamble (Wave: %d, Base: %d, Final: +%d)"), CurrentWave, BaseAmount, GambleAmount);
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

		LOG_PLAYERSTATE(FColor::Green, TEXT("Player used Common Item Choice: Added %s"), *ChosenItemID.ToString());
	}
	else
	{
		LOG_PLAYERSTATE(FColor::Red, TEXT("Server_UseCommonItemChoice: Invalid ItemID %s or not Common grade."), *ChosenItemID.ToString());
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
bool AMyPlayerState::TryUpgradeStat(EItemStatType StatToUpgrade)
{
	if (!HasAuthority()) return false;

	// [수정] GameState 가져오기 (강화 규칙 참조용)
	// (GameMode 대신 GameState를 참조하여 클라이언트 UI와 동일한 값을 보장합니다)
	AMyGameState* GameState = GetWorld()->GetGameState<AMyGameState>();
	if (!GameState) return false;

	// 강화 가능 스탯 확인 (기존 로직 유지)
	const bool bIsGoldUpgradableBasicStat = (StatToUpgrade == EItemStatType::AttackDamage ||
		StatToUpgrade == EItemStatType::AttackSpeed ||
		StatToUpgrade == EItemStatType::CritDamage);

	const bool bIsGoldUpgradableSpecialStat = (StatToUpgrade == EItemStatType::ArmorReduction ||
		StatToUpgrade == EItemStatType::SkillActivationChance);

	if (!bIsGoldUpgradableBasicStat && !bIsGoldUpgradableSpecialStat)
	{
		return false;
	}

	int32 CurrentLevel = GetStatLevel(StatToUpgrade);

	// --- [수정] 매크로 대신 GameState의 변수 사용 ---
	int32 MaxLevel = bIsGoldUpgradableBasicStat ? GameState->MaxNormalStatLevel : GameState->MaxSpecialStatLevel;
	int32 BaseCost = GameState->BaseLevelUpCost;
	int32 CostIncreaseFactor = GameState->IncreasingCostPerLevel;
	// ---------------------------------------------

	// 레벨 제한 확인
	if (CurrentLevel >= MaxLevel)
	{
		return false;
	}

	// 비용 계산
	int32 UpgradeCost = BaseCost + (CurrentLevel * CostIncreaseFactor);

	// 골드 확인 및 소모
	if (!SpendGold(UpgradeCost))
	{
		return false;
	}

	// --- [수정] 성공 확률 계산 (GameState 배열 사용) ---
	bool bUpgradeSuccess = true; // 기본 스탯은 항상 성공
	float SuccessChance = 1.0f;

	if (bIsGoldUpgradableSpecialStat) // 특수 스탯일 경우 확률 적용
	{
		// GameState의 배열 범위 확인 후 확률 가져오기
		if (GameState->SpecialStatUpgradeChances.IsValidIndex(CurrentLevel))
		{
			SuccessChance = GameState->SpecialStatUpgradeChances[CurrentLevel];
		}
		else
		{
			// 설정된 범위 밖이면 실패 처리 (혹은 0%)
			SuccessChance = 0.0f;
		}

		// 난수 생성하여 성공 여부 판정
		bUpgradeSuccess = (FMath::FRand() < SuccessChance);
	}
	// ----------------------------------------------------

	// 강화 성공 시
	if (bUpgradeSuccess)
	{
		// 1. 레벨 증가 및 변수 업데이트
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
			// 변경된 변수의 OnRep 함수 직접 호출
			switch (StatToUpgrade)
			{
			case EItemStatType::AttackDamage: OnRep_AttackDamageLevel(); break;
			case EItemStatType::AttackSpeed: OnRep_AttackSpeedLevel(); break;
			case EItemStatType::CritDamage: OnRep_CritDamageLevel(); break;
			case EItemStatType::ArmorReduction: OnRep_ArmorReductionLevel(); break;
			case EItemStatType::SkillActivationChance: OnRep_SkillActivationChanceLevel(); break;
			}

			// 로그 출력
			// RID_LOG(FColor::Green, TEXT("Upgrade Success: ..."));
		}

		// 2. 실제 스탯 적용 요청 (캐릭터 ASC에)
		ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
		if (Character)
		{
			Character->ApplyStatUpgrade(StatToUpgrade, NewLevel);
		}
		return true;
	}
	else // 강화 실패 시
	{
		// 로그 출력
		RID_LOG(FColor::Orange, TEXT("Upgrade Failed: %s (Level %d -> %d, Cost: %d, Chance: %.0f%%)"),
			*UEnum::GetValueAsString(StatToUpgrade), CurrentLevel, CurrentLevel + 1, UpgradeCost, SuccessChance * 100);
		return false;
	}
}

/**
 * @brief (서버 전용) 궁극기 스택을 추가합니다.
 */
void AMyPlayerState::AddUltimateCharge(int32 Amount)
{
	if (HasAuthority())
	{
		// 1. 현재 최대 충전량 가져오기
		int32 CurrentMaxCharge = GetMaxUltimateCharge();

		// 2. 최대치를 넘지 않도록 로직 수정
		if (UltimateCharge < CurrentMaxCharge)
		{
			UltimateCharge = FMath::Min(UltimateCharge + Amount, CurrentMaxCharge);
			OnRep_UltimateCharge();
		}
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
	// 1. UI 갱신 알림
	OnUltimateChargeChangedDelegate.Broadcast(UltimateCharge);

	int32 CurrentMaxCharge = GetMaxUltimateCharge();

	// 2. 궁극기 게이지 가득 참?
	if (UltimateCharge >= CurrentMaxCharge)
	{
		// 내(로컬) 컨트롤러인지 확인
		APlayerController* PC = GetPlayerController();
		if (PC && PC->IsLocalController())
		{
			// [핵심 변경] PlayerState의 소리가 아니라, '현재 캐릭터'의 소리를 재생
			ARamdomItemDefenseCharacter* MyCharacter = GetPawn<ARamdomItemDefenseCharacter>();

			if (MyCharacter && MyCharacter->UltimateReadySound)
			{
				UGameplayStatics::PlaySound2D(this, MyCharacter->UltimateReadySound);
			}
		}
	}
}

int32 AMyPlayerState::GetMaxUltimateCharge() const
{
	// 1. 현재 조종 중인 캐릭터를 가져옴
	const ARamdomItemDefenseCharacter* MyCharacter = GetPawn<ARamdomItemDefenseCharacter>();

	// 2. 캐릭터가 유효하면 캐릭터의 설정값을 반환
	if (MyCharacter)
	{
		return MyCharacter->GetMaxUltimateCharge();
	}

	// 3. 캐릭터가 없는 경우(로비 등) 기본값 반환 (안전장치)
	return 100;
}

/** 버튼 액션 레벨 변경 시 UI 갱신용 RepNotify */
void AMyPlayerState::OnRep_ButtonActionLevel()
{
	OnButtonActionLevelChangedDelegate.Broadcast(ButtonActionLevel);
}

/** (GameMode가 호출) 새 웨이브가 시작될 때 */
void AMyPlayerState::OnWaveStarted()
{
	if (!HasAuthority()) return;

	// 1. 실패 기록 초기화, 입력 대기 상태 해제
	bIsButtonActionSequenceFinishedThisStage = false;
	bIsWaitingForButtonActionInput = false;

	// 2. 기존 타이머 모두 중지 (안전 장치)
	GetWorld()->GetTimerManager().ClearTimer(ButtonActionTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ButtonActionInputTimeoutHandle);

	// 3. "15초" 뒤에 첫 번째 부스트 시도를 하도록 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		ButtonActionTimerHandle,
		this,
		&AMyPlayerState::TriggerButtonActionUI,
		15.0f, // (요구사항) 웨이브 시작 후 15초
		false
	);

	LOG_PLAYERSTATE(FColor::Cyan, TEXT("Wave Started. Scheduling first ButtonAction in 15s."));
}

/** (서버 전용) 15초 또는 3~5초 타이머 만료 시 호출 */
void AMyPlayerState::TriggerButtonActionUI()
{
	// (요구사항) 이미 실패했거나, (중복 방지) 이미 입력을 기다리는 중이면 실행 안 함
	if (!HasAuthority() || bIsButtonActionSequenceFinishedThisStage || bIsWaitingForButtonActionInput)
	{
		return;
	}

	// 1. 현재 레벨에 맞는 타이밍 창 가져오기
	float CurrentWindow = ButtonActionTimingWindows.IsValidIndex(ButtonActionLevel) ? ButtonActionTimingWindows[ButtonActionLevel] : 1.0f;

	// 2. 랜덤 키 선택 (0=Q ~ 7=F)
	const int32 RandomIndex = FMath::RandRange(0, 7);
	CurrentRequiredButtonActionKey = static_cast<EButtonActionKey>(RandomIndex);

	LOG_PLAYERSTATE(FColor::Cyan, TEXT("TriggerButtonActionUI: Showing QTE (Level: %d, KeyIndex: %d, Window: %.1fs)"), ButtonActionLevel, RandomIndex, CurrentWindow);
	
	// 3. 입력 대기 상태로 전환
	bIsWaitingForButtonActionInput = true;

	// 4. PlayerController를 통해 클라이언트에게 UI 표시 요청
	ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(GetPlayerController());
	if (PC)
	{
		PC->Client_OnShowButtonActionUI(CurrentWindow, CurrentRequiredButtonActionKey);
	}

	// 5. (실패 판정 타이머)
	GetWorld()->GetTimerManager().SetTimer(
		ButtonActionInputTimeoutHandle,
		this,
		&AMyPlayerState::OnButtonActionTimeout,
		CurrentWindow + 0.1f, // (네트워크 지연 고려 0.1초)
		false
	);
}

/** (서버 전용) 플레이어가 입력을 놓쳤을 때 (타임아웃 실패) */
void AMyPlayerState::OnButtonActionTimeout()
{
	if (!HasAuthority()) return;

	// (중복 방지) 성공/실패 RPC가 먼저 도착해서 이미 처리됐다면 무시
	if (!bIsWaitingForButtonActionInput) return;

	bIsWaitingForButtonActionInput = false; // 상태 해제

	LOG_PLAYERSTATE(FColor::Red, TEXT("Button Action FAILED (Timeout). Level reset to 0."));

	Client_NotifyButtonActionResult(false, -1);
	// (요구사항) 실패 기록 (이번 스테이지 끝날 때까지)
	bIsButtonActionSequenceFinishedThisStage = true;

	// (요구사항) 난이도(레벨) 0으로 초기화
	ButtonActionLevel = 0;
	OnRep_ButtonActionLevel(); // UI 갱신 (0)

	// (요구사항) 다음 타이머를 예약하지 않음 (이번 스테이지 기회 끝)
	GetWorld()->GetTimerManager().ClearTimer(ButtonActionTimerHandle);
}

/** (클라 -> 서버) 플레이어가 "성공(정확한 키)"을 보고 */
void AMyPlayerState::Server_ReportButtonActionSuccess_Implementation()
{
	// (중복 방지) 타임아웃이 먼저 발생했거나, 잘못된 호출이면 무시
	if (!HasAuthority() || !bIsWaitingForButtonActionInput) return;

	bIsWaitingForButtonActionInput = false; // 상태 해제
	GetWorld()->GetTimerManager().ClearTimer(ButtonActionInputTimeoutHandle);

	// 1. 부스트 레벨 1단계 올리기 (최대 5)
	ButtonActionLevel = FMath::Min(ButtonActionLevel + 1, 5);
	LOG_PLAYERSTATE(FColor::Green, TEXT("Button Action SUCCESS. Level up to %d."), ButtonActionLevel);

	// 2. (5단계 보상)
	if (ButtonActionLevel == 5)
	{
		RID_LOG(FColor::Magenta, TEXT("PlayerState: Reached MAX ACTION (Level 5)! Applying RANDOM reward."));

		bIsButtonActionSequenceFinishedThisStage = true;
		ButtonActionLevel = 0;

		// [ ★★★ 랜덤 버프 로직 구현 ★★★ ]
		int32 RandomRewardIndex = -1;

		if (ButtonActionRewardBuffs.Num() > 0)
		{
			// 1. 0 ~ (개수-1) 사이 랜덤 인덱스 선택
			RandomRewardIndex = FMath::RandRange(0, ButtonActionRewardBuffs.Num() - 1);

			// 2. 해당 인덱스의 GE 적용
			if (ButtonActionRewardBuffs[RandomRewardIndex])
			{
				if (ARamdomItemDefenseCharacter* Char = GetPawn<ARamdomItemDefenseCharacter>())
				{
					if (UAbilitySystemComponent* ASC = Char->GetAbilitySystemComponent())
					{
						ASC->ApplyGameplayEffectToSelf(ButtonActionRewardBuffs[RandomRewardIndex]->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
						RID_LOG(FColor::Cyan, TEXT("Applied Reward Buff Index: %d"), RandomRewardIndex);
					}
				}
			}
		}

		// 3. 클라이언트에 성공 및 보상 인덱스 알림
		Client_NotifyButtonActionResult(true, RandomRewardIndex);
	}
	else
	{
		// 1~4단계 성공 (보상 없음 -> 인덱스 -1)
		Client_NotifyButtonActionResult(true, -1);
		// 3~5초 랜덤 간격 뒤에 다음 부스트 시도 예약
		float RandomInterval = FMath::RandRange(3.0f, 5.0f);
		GetWorld()->GetTimerManager().SetTimer(
			ButtonActionTimerHandle,
			this,
			&AMyPlayerState::TriggerButtonActionUI,
			RandomInterval,
			false
		);
	}
	// 3. 레벨 UI 갱신
	OnRep_ButtonActionLevel();
}

void AMyPlayerState::Server_ReportButtonActionFailure_Implementation()
{
	// (중복 방지) 타임아웃이 먼저 발생했거나, 잘못된 호출이면 무시
	if (!HasAuthority() || !bIsWaitingForButtonActionInput) return;

	// 1. 상태 해제 및 타임아웃 타이머 중지
	bIsWaitingForButtonActionInput = false;
	GetWorld()->GetTimerManager().ClearTimer(ButtonActionInputTimeoutHandle);

	// 2. 클라이언트에게 실패 알림 (UI 갱신)
	Client_NotifyButtonActionResult(false);

	// --- [ ★★★ 수정: OnButtonActionTimeout() 호출 대신 직접 로직 수행 ★★★ ] ---

	// 3. 로그 출력
	LOG_PLAYERSTATE(FColor::Orange, TEXT("Button Action FAILED (Wrong Key). Level reset to 0."));

	// 4. 이번 스테이지 실패 기록
	bIsButtonActionSequenceFinishedThisStage = true;

	// 5. 난이도(레벨) 0으로 초기화 (핵심!)
	ButtonActionLevel = 0;
	OnRep_ButtonActionLevel(); // UI 갱신

	// 6. 다음 타이머 예약 취소 (이번 스테이지 기회 끝)
	GetWorld()->GetTimerManager().ClearTimer(ButtonActionTimerHandle);

	// -----------------------------------------------------------------------
}

void AMyPlayerState::Client_NotifyButtonActionResult_Implementation(bool bWasSuccess, int32 RewardIndex)
{
	ARamdomItemDefensePlayerController* PC = Cast<ARamdomItemDefensePlayerController>(GetPlayerController());
	if (PC)
	{
		// Controller에게 보상 인덱스까지 전달
		PC->OnButtonActionResult(bWasSuccess, RewardIndex);
	}
}

void AMyPlayerState::Server_SetSelectedCharacter_Implementation(TSubclassOf<APawn> NewClass)
{
	SelectedCharacterClass = NewClass;

	UE_LOG(LogTemp, Warning, TEXT("Server: Player %s selected %s"), *GetPlayerName(), *GetNameSafe(NewClass));
}

void AMyPlayerState::Server_SetPlayerName_Implementation(const FString& NewName)
{
	SetPlayerName(NewName);

	RID_LOG(FColor::Green, TEXT("Server: Updated Player Name to '%s'"), *NewName);
}

// [추가] 서버 구현: 상태 변경 및 알림
void AMyPlayerState::Server_SetIsReady_Implementation(bool bReady)
{
	bIsReady = bReady;
	OnIsReadyChangedDelegate.Broadcast(bIsReady);
	RID_LOG(FColor::Green, TEXT("Server: Player %s Ready Status -> %s"), *GetPlayerName(), bReady ? TEXT("TRUE") : TEXT("FALSE"));
}

// [추가] 클라이언트 동기화 알림
void AMyPlayerState::OnRep_IsReady()
{
	OnIsReadyChangedDelegate.Broadcast(bIsReady);
}