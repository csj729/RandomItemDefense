#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ItemTypes.h"
#include "MyPlayerState.generated.h"

class AMonsterSpawner;
// --- [ ★★★ 코드 추가 ★★★ ] ---
class AMyGameState; // AMyGameState 전방 선언
// --- [ 코드 추가 끝 ] ---

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

	// --- [코드 추가] 라운드 선택 관련 ---
public:
	/** @brief 현재 남은 라운드 선택 횟수를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Player State")
	int32 GetChoiceCount() const { return ChoiceCount; }

	/** @brief (서버 전용) 라운드 선택 횟수를 설정합니다. */
	void AddChoiceCount(int32 Count);

	/** (UI에서 호출) 라운드 선택(아이템/골드)을 사용합니다. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseRoundChoice(bool bChoseItemGacha);

	/** UI 바인딩용 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnIntChangedDelegate OnChoiceCountChangedDelegate;

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

protected:
	/** 현재 남은 라운드 선택 횟수 */
	UPROPERTY(ReplicatedUsing = OnRep_ChoiceCount)
	int32 ChoiceCount;

	/** 클라이언트에서 ChoiceCount가 복제되었을 때 호출됩니다. */
	UFUNCTION()
	void OnRep_ChoiceCount();
	// ------------------------------------
};