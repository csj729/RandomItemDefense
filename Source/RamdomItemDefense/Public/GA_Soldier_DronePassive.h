// Public/GA_Soldier_DronePassive.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Soldier_DronePassive.generated.h"

class APawn;
class UGameplayEffect;

/**
 * 솔져 스킬 2: 드론 소환 (패시브)
 * 게임 시작 시 드론을 소환하고, 주인 스탯의 1/4을 이전합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API UGA_Soldier_DronePassive : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Soldier_DronePassive();

protected:
	// --- [ ★★★ 수정 ★★★ ] ---
	/** 어빌리티가 아바타에 적용(부여)될 때 호출됩니다. (패시브 활성화용) */
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	// --- [ ★★★ 수정 끝 ★★★ ] ---

	/** 어빌리티가 활성화될 때 (OnAvatarSet에서 호출됨) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 어빌리티가 제거될 때 (캐릭터 사망/게임 종료 시) 호출됨 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** (BP 설정) 스폰할 드론의 폰 클래스 (ASoldier_Drone의 BP) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TSubclassOf<APawn> DroneClassToSpawn;

	/** (BP 설정) 드론에게 스탯(1/4)을 이전할 때 사용할 GE (SetByCaller) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TSubclassOf<UGameplayEffect> DroneStatInitEffect;

	/** (BP 설정) 드론에게 부여할 어빌리티 목록 (드론용 GA_AttackSelector, GA_Soldier_Grenade_Drone 등) */
	UPROPERTY(EditDefaultsOnly, Category = "Config|Drone")
	TArray<TSubclassOf<UGameplayAbility>> DroneAbilities;

private:
	/** 스폰된 드론의 참조 (제거용) */
	UPROPERTY()
	TObjectPtr<APawn> SpawnedDrone;
};