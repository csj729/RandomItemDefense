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


private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
};

