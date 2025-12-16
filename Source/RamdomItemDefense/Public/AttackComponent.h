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

	virtual void Initialize(UAbilitySystemComponent* InASC, const UMyAttributeSet* InAttributeSet);

	UFUNCTION(Server, Reliable)
	void OrderAttack(AActor* Target);

	void ClearAllTargets();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attack|Reference")
	TObjectPtr<ARamdomItemDefenseCharacter> OwnerCharacter;

protected:
	virtual void BeginPlay() override;

private:
	void FindTarget();

	void PerformAttack();

	void ExecuteAttackLogic(AActor* TargetActor);

	void OnAttackSpeedChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	TObjectPtr<AActor> ManualTarget;

	UPROPERTY()
	TObjectPtr<AActor> AutoTarget;

	UPROPERTY()
	TObjectPtr<AActor> PendingManualTarget;

	// --- [ System : Timers ] ---
	FTimerHandle FindTargetTimerHandle;
	FTimerHandle PerformAttackTimerHandle;

	// --- [ Cached Dependencies ] ---
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UMyAttributeSet> AttributeSet;
};