// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.generated.h"

// 전방 선언
class UAttackComponent;
class UInventoryComponent;

UCLASS(Blueprintable)
class ARamdomItemDefenseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ARamdomItemDefenseCharacter();

	virtual void Tick(float DeltaSeconds) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	FORCEINLINE const UMyAttributeSet* GetAttributeSet() const { return AttributeSet; }
	FORCEINLINE UAttackComponent* GetAttackComponent() const { return AttackComponent; }
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** (서버 전용) PlayerState로부터 스탯 강화 적용 요청을 받습니다. */
	void ApplyStatUpgrade(EItemStatType StatType, int32 NewLevel);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttackComponent> AttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMyAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TSubclassOf<UGameplayAbility> DefaultAttackAbility;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultStatsEffect;

	void ApplyDefaultStats();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> ManualTarget;

	/** 각 스탯 강화를 위한 GameplayEffect 클래스 (에디터에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Upgrade")
	TMap<EItemStatType, TSubclassOf<class UGameplayEffect>> UpgradeEffects;


private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
};

