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
class UAnimMontage;
class UWidgetComponent;
class UUserWidget;
class UMyAttributeSet;

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

	/**
	 * @brief 설정된 공격 몽타주 배열에서 랜덤하게 하나를 골라 반환합니다.
	 * @return 재생할 몽타주 (없으면 nullptr)
	 */
	UAnimMontage* GetRandomAttackMontage() const;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAttack(UAnimMontage* MontageToPlay, FRotator TargetRotation);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleAtLocation(UParticleSystem* EmitterTemplate, FVector Location, FRotator Rotation, FVector Scale);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttackComponent> AttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/**
	 * @brief 재생할 기본 공격 몽타주 '배열'입니다.
	 * 블루프린트 클래스 디폴트에서 설정해야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<TObjectPtr<UAnimMontage>> DefaultAttackMontages; // 단일 변수에서 TArray로

	UPROPERTY()
	TObjectPtr<UMyAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultStatsEffect;

	/**
	 * @brief 캐릭터가 기본적으로 보유할 어빌리티 목록입니다.
	 * (GA_AttackSelector, GA_MeteorStrike_BP, GA_ArcaneBind_BP 등)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;

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

