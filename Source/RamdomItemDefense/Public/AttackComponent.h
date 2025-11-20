// Public/AttackComponent.h (수정)

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

	/** 수동으로 대상을 공격하도록 명령합니다. */
	UFUNCTION(Server, Reliable)
	void OrderAttack(AActor* Target);

	/** 수동 및 자동 타겟을 모두 해제합니다. */
	void ClearAllTargets();

	/**
	 * @brief (수정) ASC와 AttributeSet을 받아 컴포넌트를 초기화하고 타이머를 시작합니다.
	 * Character의 BeginPlay와 Drone의 BeginPlay에서 호출됩니다.
	 */
	virtual void Initialize(UAbilitySystemComponent* InASC, const UMyAttributeSet* InAttributeSet);

	/** (수정) 소유자가 캐릭터일 경우에만 이 변수가 설정됩니다. 드론의 경우 nullptr입니다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<ARamdomItemDefenseCharacter> OwnerCharacter;

protected:
	/** (수정) BeginPlay는 이제 오너가 캐릭터일 경우에만 Initialize를 자동 호출합니다. */
	virtual void BeginPlay() override;

private:
	/** 자동으로 가장 가까운 적을 찾습니다. */
	void FindTarget();

	/** 현재 타겟을 향해 공격을 수행합니다. */
	void PerformAttack();

	/** 공격 속성 변경 시 타이머를 재설정하기 위한 콜백 함수 */
	void OnAttackSpeedChanged(const FOnAttributeChangeData& Data);

	/** 수동 지정 타겟 */
	UPROPERTY()
	TObjectPtr<AActor> ManualTarget;

	/** 자동 감지 타겟 */
	UPROPERTY()
	TObjectPtr<AActor> AutoTarget;

	UPROPERTY()
	TObjectPtr<AActor> PendingManualTarget;

	FTimerHandle FindTargetTimerHandle;
	FTimerHandle PerformAttackTimerHandle;

	/** 캐시된 AbilitySystemComponent (캐릭터 또는 드론의 ASC) */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** 캐시된 AttributeSet (캐릭터 또는 드론의 AttributeSet) */
	UPROPERTY()
	TObjectPtr<const UMyAttributeSet> AttributeSet;
};