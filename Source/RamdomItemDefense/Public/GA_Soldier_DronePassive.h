// Public/GA_Soldier_DronePassive.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Soldier_DronePassive.generated.h"

class APawn;
class UGameplayEffect;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;
class ARamdomItemDefenseCharacter; // (추가)

/**
 * 솔져 스킬 2: 드론 소환 (패시브)
 * 게임 시작 시 드론을 소환하고, 주인 스탯의 1/5을 실시간으로 이전합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_DronePassive : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Soldier_DronePassive();

protected:
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** (BP 설정) 스폰할 드론의 폰 클래스 (ASoldierDrone의 BP) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TSubclassOf<APawn> DroneClassToSpawn; // (수정) ASoldierDrone -> APawn

	/** (BP 설정) 드론에게 스탯(1/5)을 이전할 때 사용할 GE (SetByCaller, Instant) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TSubclassOf<UGameplayEffect> DroneStatInitEffect;

	/** (BP 설정) 드론에게 부여할 어빌리티 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TArray<TSubclassOf<UGameplayAbility>> DroneAbilities;

private:
	/** 스폰된 드론의 참조 (제거용) */
	UPROPERTY()
	TObjectPtr<APawn> SpawnedDrone; // (수정) ASoldierDrone -> APawn

	/** (내부 캐시) 소유자(솔져)의 ASC */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC_Cache;

	/** (내부 캐시) 스폰된 드론의 ASC */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> DroneASC_Cache;

	/** 드론에게 적용된 스탯 GE 핸들 (갱신 시 제거용) */
	FActiveGameplayEffectHandle DroneStatEffectHandle;

	/** 소유자 스탯 변경 델리게이트 핸들 (해제용) */
	TArray<FDelegateHandle> AttributeDelegateHandles;

	void OnOwnerStatChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void UpdateDroneStats();
};