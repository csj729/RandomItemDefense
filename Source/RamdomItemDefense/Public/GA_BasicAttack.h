// Source/RamdomItemDefense/Public/GA_BasicAttack.h (수정)

#pragma once

#include "RamdomItemDefense.h" // 프로젝트 헤더 먼저
#include "CoreMinimal.h"
#include "GA_BaseSkill.h"
#include "GameplayTagContainer.h"
#include "GA_BasicAttack.generated.h"

class UGameplayEffect;

UCLASS()
class RAMDOMITEMDEFENSE_API UGA_BasicAttack : public UGA_BaseSkill // 부모 클래스 변경
	// --- [ ★★★ 수정 끝 ★★★ ] ---
{
	GENERATED_BODY()

public:
	UGA_BasicAttack();

protected:
	/** 어빌리티 활성화 시 (Event.Attack.Execute.Basic 수신 시) */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** (블루프린트에서 설정) SetByCaller로 데미지 값을 전달할 때 사용할 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	FGameplayTag DamageDataTag;
};