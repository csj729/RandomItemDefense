// Source/RamdomItemDefense/Public/AttackComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

// 전방 선언
class ARamdomItemDefenseCharacter;
class UAbilitySystemComponent;
class UMyAttributeSet;
struct FOnAttributeChangeData;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RAMDOMITEMDEFENSE_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttackComponent();

	// --- [ Initialization ] ---
	/** * 컴포넌트 초기화 및 타이머 시작.
	 * Character나 Drone의 BeginPlay에서 호출됩니다.
	 */
	virtual void Initialize(UAbilitySystemComponent* InASC, const UMyAttributeSet* InAttributeSet);

	// --- [ Public API : Command ] ---
	/** (RPC) 서버에게 특정 대상을 공격하도록 명령 */
	UFUNCTION(Server, Reliable)
	void OrderAttack(AActor* Target);

	/** 모든 타겟(수동/자동) 해제 및 공격 중지 */
	void ClearAllTargets();

	// --- [ References ] ---
	/** 소유자 캐릭터 (드론인 경우 nullptr일 수 있음) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attack|Reference")
	TObjectPtr<ARamdomItemDefenseCharacter> OwnerCharacter;

protected:
	// --- [ Lifecycle ] ---
	virtual void BeginPlay() override;

private:
	// --- [ Internal Logic : AI & Execution ] ---
	/** 사거리 내 가장 가까운 적을 탐색 (자동 공격용) */
	void FindTarget();

	/** 현재 타겟(수동 우선, 없으면 자동)을 향해 공격 시도 (이동 or 공격) */
	void PerformAttack();

	/** 실제 공격 실행 (회전, 몽타주 재생, GAS 이벤트 전송) */
	void ExecuteAttackLogic(AActor* TargetActor);

	// --- [ Event Handlers ] ---
	/** 공격 속도 변경 시 타이머 주기를 갱신하는 콜백 */
	void OnAttackSpeedChanged(const FOnAttributeChangeData& Data);

	// --- [ Internal State : Targeting ] ---
	/** 플레이어가 직접 지정한 타겟 */
	UPROPERTY()
	TObjectPtr<AActor> ManualTarget;

	/** AI가 감지한 타겟 */
	UPROPERTY()
	TObjectPtr<AActor> AutoTarget;

	/** (네트워크 동기화용) 대기 중인 수동 타겟 */
	UPROPERTY()
	TObjectPtr<AActor> PendingManualTarget;

	// --- [ System : Timers ] ---
	FTimerHandle FindTargetTimerHandle;
	FTimerHandle PerformAttackTimerHandle;

	// --- [ Cached Dependencies ] ---
	/** GAS 컴포넌트 캐싱 */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** 속성 세트 캐싱 */
	UPROPERTY()
	TObjectPtr<const UMyAttributeSet> AttributeSet;
};