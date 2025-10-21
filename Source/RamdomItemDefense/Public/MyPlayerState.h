#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ItemTypes.h"
#include "MyPlayerState.generated.h"

class AMonsterSpawner;

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

protected:
	/** 현재 남은 라운드 선택 횟수 */
	UPROPERTY(ReplicatedUsing = OnRep_ChoiceCount)
	int32 ChoiceCount;

	/** 클라이언트에서 ChoiceCount가 복제되었을 때 호출됩니다. */
	UFUNCTION()
	void OnRep_ChoiceCount();
	// ------------------------------------
};