// Source/RamdomItemDefense/Public/MyPlayerState.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ItemTypes.h"
#include "MyPlayerState.generated.h"

class AMonsterSpawner;
class AMyGameState; // AMyGameState 전방 선언

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
};