// Source/RamdomItemDefense/Public/MyPlayerState.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ItemTypes.h"
#include "RamdomItemDefense.h" 
#include "MyPlayerState.generated.h"

class AMonsterSpawner;
class AMyGameState; // AMyGameState 전방 선언

// 2. PlayerState 전용 *개별 스위치*를 정의합니다.
#define ENABLE_PLAYERSTATE_DEBUG 1

// 3. PlayerState 전용 로그 카테고리를 선언합니다.
DECLARE_LOG_CATEGORY_EXTERN(LogRID_PlayerState, Log, All);

// 4. 전용 커스텀 로그 매크로를 정의합니다.
#if ENABLE_RID_DEBUG && ENABLE_PLAYERSTATE_DEBUG
#include "Engine/Engine.h" // GEngine 사용을 위해 포함
#define LOG_PLAYERSTATE(Color, Format, ...) \
	{ \
		if (GEngine) \
		{ \
			FString Msg = FString::Printf(Format, ##__VA_ARGS__); \
			GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, FString::Printf(TEXT("[PState] %s"), *Msg)); \
			UE_LOG(LogRID_PlayerState, Log, TEXT("%s"), *Msg); \
		} \
	}
#else
#define LOG_PLAYERSTATE(Color, Format, ...) (void)0
#endif

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatLevelChangedDelegate, EItemStatType, StatType, int32, NewLevel);

UCLASS()
class RAMDOMITEMDEFENSE_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMyPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- 골드 관련 (수정됨) ---
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetGold() const { return Gold; }
	void AddGold(int32 Amount); // 서버 전용

	/** (서버 전용) 골드를 소모합니다. 성공 시 true 반환 */
	bool SpendGold(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnGoldChangedDelegate;

	/** UI 바인딩용 델리게이트 (스포너가 할당/복제되었을 때 호출) */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnSpawnerAssignedDelegate; // (FOnIntChangedDelegate를 재활용, 값은 의미 없음)

	/** 이 플레이어에게 할당된 몬스터 스포너입니다. */
	UPROPERTY(ReplicatedUsing = OnRep_MySpawner, BlueprintReadOnly, Category = "Player State")
	TObjectPtr<AMonsterSpawner> MySpawner;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Gold)
	int32 Gold;
	UFUNCTION()
	void OnRep_Gold();

	UFUNCTION()
	void OnRep_MySpawner();

	// --- [코드 수정] 라운드 선택 관련 ---
public:
	/** @brief 현재 남은 라운드 선택 횟수(뽑기/도박)를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetChoiceCount() const { return ChoiceCount; }

	/** @brief (서버 전용) 라운드 선택 횟수(뽑기/도박)를 추가합니다. */
	void AddChoiceCount(int32 Count);

	/** (UI에서 호출) 라운드 선택(아이템/골드)을 사용합니다. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseRoundChoice(bool bChoseItemGacha);

	/** UI 바인딩용 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnChoiceCountChangedDelegate;

	// --- [ ★★★ 코드 추가 (흔함 아이템 선택) ★★★ ] ---
public:
	/** @brief 현재 남은 '흔함 아이템 선택권' 횟수를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetCommonItemChoiceCount() const { return CommonItemChoiceCount; }

	/** @brief (서버 전용) '흔함 아이템 선택권' 횟수를 추가합니다. (보스 처치 시 호출) */
	void AddCommonItemChoice(int32 Count);

	/** (새 UI에서 호출) '흔함 아이템 선택권'을 사용하고 아이템을 획득합니다. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseCommonItemChoice(FName ChosenItemID);

	/** (새 UI 바인딩용) '흔함 아이템 선택권' 횟수 변경 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnCommonItemChoiceCountChangedDelegate;
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---


	// --- 스탯 강화 관련 ---
public:
	/** (UI에서 호출) 서버에 스탯 강화를 요청합니다. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Upgrade")
	void Server_RequestStatUpgrade(EItemStatType StatToUpgrade);

	/** 특정 스탯의 현재 강화 단계를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetStatLevel(EItemStatType StatType) const;

	/** UI 바인딩용: 특정 스탯의 레벨이 변경될 때 호출됩니다. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStatLevelChangedDelegate OnStatLevelChangedDelegate;

protected:
	/** 각 스탯의 현재 강화 단계 (0부터 시작) */
	UPROPERTY(ReplicatedUsing = OnRep_AttackDamageLevel)
	int32 AttackDamageLevel;
	UPROPERTY(ReplicatedUsing = OnRep_AttackSpeedLevel)
	int32 AttackSpeedLevel;
	UPROPERTY(ReplicatedUsing = OnRep_CritDamageLevel)
	int32 CritDamageLevel;
	UPROPERTY(ReplicatedUsing = OnRep_ArmorReductionLevel)
	int32 ArmorReductionLevel;
	UPROPERTY(ReplicatedUsing = OnRep_SkillActivationChanceLevel)
	int32 SkillActivationChanceLevel;

	/** 클라이언트에서 StatLevels가 복제되었을 때 호출됩니다. */
	UFUNCTION() void OnRep_AttackDamageLevel();
	UFUNCTION() void OnRep_AttackSpeedLevel();
	UFUNCTION() void OnRep_CritDamageLevel();
	UFUNCTION() void OnRep_ArmorReductionLevel();
	UFUNCTION() void OnRep_SkillActivationChanceLevel();

	/** (서버 전용) 실제 스탯 강화를 시도하고 결과를 반환합니다. */
	bool TryUpgradeStat(EItemStatType StatToUpgrade);

	// --- [ ★★★ 코드 추가 (궁극기 스택) ★★★ ] ---
public:
	/** @brief 현재 궁극기 스택을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Player State|Ultimate")
	int32 GetUltimateCharge() const { return UltimateCharge; }

	/** * @brief 궁극기 최대 스택 값(C++ 매크로)을 반환합니다. (BP용)
	 * GA_UltimateSkill의 CanActivate 등에서 이 값을 사용합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "Player State|Ultimate")
	int32 GetMaxUltimateCharge() const;

	/** @brief (서버 전용) 궁극기 스택을 1 증가시킵니다. (AttackComponent에서 호출) */
	void AddUltimateCharge(int32 Amount);

	/** @brief (서버 전용) 궁극기 스택을 0으로 리셋합니다. (GA_Ultimate에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Player State|Ultimate")
	void ResetUltimateCharge();

	/** (UI 바인딩용) 궁극기 스택 변경 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnUltimateChargeChangedDelegate;

protected:
	/** 현재 궁극기 스택 (0 ~ MAX_ULTIMATE_CHARGE) */
	UPROPERTY(ReplicatedUsing = OnRep_UltimateCharge)
	int32 UltimateCharge;

	/** 클라이언트에서 UltimateCharge가 복제되었을 때 호출됩니다. */
	UFUNCTION()
	void OnRep_UltimateCharge();

protected:
	/** 현재 남은 라운드 선택 횟수 (뽑기/도박) */
	UPROPERTY(ReplicatedUsing = OnRep_ChoiceCount)
	int32 ChoiceCount;

	/** 클라이언트에서 ChoiceCount가 복제되었을 때 호출됩니다. */
	UFUNCTION()
	void OnRep_ChoiceCount();

	// --- [ ★★★ 코드 추가 (흔함 아이템 선택) ★★★ ] ---
protected:
	/** 현재 남은 '흔함 아이템 선택권' 횟수 (보스 보상) */
	UPROPERTY(ReplicatedUsing = OnRep_CommonItemChoiceCount)
	int32 CommonItemChoiceCount;

	/** 클라이언트에서 CommonItemChoiceCount가 복제되었을 때 호출됩니다. */
	UFUNCTION()
	void OnRep_CommonItemChoiceCount();
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

// --- [ ★★★ 버튼 액션 (QTE) 시스템 ★★★ ] ---
protected:
	/** 현재 버튼 액션 단계 (0~5) */
	UPROPERTY(ReplicatedUsing = OnRep_ButtonActionLevel)
	int32 ButtonActionLevel;

	/** 이번 스테이지에서 버튼 액션 시퀀스가 (성공이든 실패든) 종료되었는지 여부 */
	UPROPERTY(Replicated)
	bool bIsButtonActionSequenceFinishedThisStage;

	/** 버튼 액션 타이머 (15초 대기, 3~5초 간격) */
	FTimerHandle ButtonActionTimerHandle;

	/** 버튼 액션 입력 대기(실패 판정) 타이머 */
	FTimerHandle ButtonActionInputTimeoutHandle;

	/** 버튼 액션 레벨별 입력 허용 시간 (초) (0~5단계) */
	TArray<float> ButtonActionTimingWindows = { 2.0f, 1.6f, 1.2f, 1.0f, 0.7f};

	/** 현재 시퀀스에서 요구되는 키 (서버 전용 상태) */
	EButtonActionKey CurrentRequiredButtonActionKey;

	/** 현재 클라이언트의 입력을 기다리는 중인지 (서버 전용 상태) */
	bool bIsWaitingForButtonActionInput;

	/** 버튼 액션 레벨 변경 시 UI 갱신용 RepNotify */
	UFUNCTION()
	void OnRep_ButtonActionLevel();

public:
	/** UI 바인딩용: 버튼 액션 레벨 변경 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnButtonActionLevelChangedDelegate;

	/** (GameMode가 호출) 새 웨이브가 시작될 때 이 플레이어의 버튼 액션 상태를 리셋합니다. */
	void OnWaveStarted();

	/** (서버 전용) ButtonActionTimerHandle에 의해 호출 (UI 표시 트리거) */
	void TriggerButtonActionUI();

	/** (서버 전용) 플레이어가 입력을 놓쳤을 때 (실패 판정) */
	void OnButtonActionTimeout();

	/** (클라 -> 서버) 플레이어가 "성공(정확한 키)"을 보고 */
	UFUNCTION(Server, Reliable)
	void Server_ReportButtonActionSuccess();

	/** (클라 -> 서버) 플레이어가 "실패(틀린 키)"를 보고 */
	UFUNCTION(Server, Reliable)
	void Server_ReportButtonActionFailure();

	UFUNCTION(Client, Reliable)
	void Client_NotifyButtonActionResult(bool bWasSuccess, int32 RewardIndex = -1);

	// [ ★★★ 수정: 변수 변경 ★★★ ]
	/** 5단계 보상으로 적용할 랜덤 버프 목록 (3개 등록 필수) */
	UPROPERTY(EditDefaultsOnly, Category = "Button Action")
	TArray<TSubclassOf<class UGameplayEffect>> ButtonActionRewardBuffs;

};