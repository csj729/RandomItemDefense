// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

// ���� ����
class ARamdomItemDefenseCharacter;
class UAbilitySystemComponent;
class UMyAttributeSet;
struct FOnAttributeChangeData; // FOnAttributeChangeData ����ü ���� ����

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

	// ���� �Ӽ� ���� �� ȣ��� �ݹ� �Լ�
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

