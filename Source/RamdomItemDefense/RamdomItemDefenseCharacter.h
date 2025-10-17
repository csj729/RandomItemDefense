// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "MyAttributeSet.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "RamdomItemDefenseCharacter.generated.h"

UCLASS(Blueprintable)
class ARamdomItemDefenseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ARamdomItemDefenseCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;

	// IAbilitySystemInterface ���� �Լ�. ���� ĳ���Ͱ� ������ ASC�� ���� ��ȯ�մϴ�.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ����/Ŭ���̾�Ʈ �ʱ�ȭ �Լ��� PlayerState�� �ƴ� Character���� ���� ó���մϴ�.
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override; // PlayerState ���� �� ������ �ʿ��� �� �ֽ��ϴ�.

	// �������Ʈ�� ��Ʈ�ѷ����� ȣ���� Ÿ�� ����/���� �Լ�
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetManualTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ClearManualTarget();

	// ��Ʈ�ѷ��� ȣ���� ���ο� �Լ���
	void SetPendingManualTarget(AActor* NewTarget);
	void ClearAllTargets();

	// AI �̵��� �Ϸ�Ǹ� ȣ��� �Լ�
	void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	FORCEINLINE const UMyAttributeSet* GetAttributeSet() const { return AttributeSet; }


protected:
	// --- GAS �ٽ� ������Ʈ ---
	// ĳ���Ͱ� ���� AbilitySystemComponent�� �����մϴ�.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// ĳ���Ͱ� ���� AttributeSet�� �����մϴ�.
	UPROPERTY()
	TObjectPtr<UMyAttributeSet> AttributeSet;

	// -- ���� ���� --
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Combat")
	TObjectPtr<AActor> ManualTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> AutoTarget;

	// '������ �ǵ�'�� ������ ���� ��Ÿ����� ������ ���� Ÿ��
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> PendingManualTarget;

	// -- GAS ���� ���� --
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TSubclassOf<UGameplayAbility> DefaultAttackAbility;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultStatsEffect;

	// -- Ÿ�̸� �ڵ� --
	FTimerHandle FindTargetTimerHandle;
	FTimerHandle PerformAttackTimerHandle;

	// -- �ٽ� ���� �Լ� --
	void FindTarget();
	void PerformAttack();
	void ApplyDefaultStats();

	// ���� ������ ���� GetLifetimeReplicatedProps �Լ� ������
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

};