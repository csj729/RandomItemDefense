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

	// IAbilitySystemInterface 구현 함수. 이제 캐릭터가 소유한 ASC를 직접 반환합니다.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 서버/클라이언트 초기화 함수는 PlayerState가 아닌 Character에서 직접 처리합니다.
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override; // PlayerState 복제 시 여전히 필요할 수 있습니다.

	// 블루프린트나 컨트롤러에서 호출할 타겟 설정/해제 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetManualTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ClearManualTarget();

	// 컨트롤러가 호출할 새로운 함수들
	void SetPendingManualTarget(AActor* NewTarget);
	void ClearAllTargets();

	// AI 이동이 완료되면 호출될 함수
	void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	FORCEINLINE const UMyAttributeSet* GetAttributeSet() const { return AttributeSet; }


protected:
	// --- GAS 핵심 컴포넌트 ---
	// 캐릭터가 직접 AbilitySystemComponent를 소유합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 캐릭터가 직접 AttributeSet을 소유합니다.
	UPROPERTY()
	TObjectPtr<UMyAttributeSet> AttributeSet;

	// -- 상태 변수 --
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Combat")
	TObjectPtr<AActor> ManualTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> AutoTarget;

	// '공격할 의도'는 있지만 아직 사거리에는 들어오지 않은 타겟
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> PendingManualTarget;

	// -- GAS 관련 설정 --
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TSubclassOf<UGameplayAbility> DefaultAttackAbility;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultStatsEffect;

	// -- 타이머 핸들 --
	FTimerHandle FindTargetTimerHandle;
	FTimerHandle PerformAttackTimerHandle;

	// -- 핵심 로직 함수 --
	void FindTarget();
	void PerformAttack();
	void ApplyDefaultStats();

	// 변수 복제를 위해 GetLifetimeReplicatedProps 함수 재정의
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

};