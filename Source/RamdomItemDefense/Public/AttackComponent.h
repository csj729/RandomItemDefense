// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

// 전방 선언
class ARamdomItemDefenseCharacter;
class UAbilitySystemComponent;
class UMyAttributeSet;
struct FOnAttributeChangeData; // FOnAttributeChangeData 구조체 전방 선언

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RAMDOMITEMDEFENSE_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttackComponent();

	void OrderAttack(AActor* Target);
	void ClearAllTargets();

protected:
	virtual void BeginPlay() override;

private:
	void FindTarget();
	void PerformAttack();

	// 공격 속성 변경 시 호출될 콜백 함수
	void OnAttackSpeedChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	TObjectPtr<AActor> ManualTarget;
	UPROPERTY()
	TObjectPtr<AActor> AutoTarget;
	UPROPERTY()
	TObjectPtr<AActor> PendingManualTarget;

	FTimerHandle FindTargetTimerHandle;
	FTimerHandle PerformAttackTimerHandle;

	UPROPERTY()
	TObjectPtr<ARamdomItemDefenseCharacter> OwnerCharacter;
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY()
	TObjectPtr<const UMyAttributeSet> AttributeSet;
};

