// Private/GA_AttackSelector.cpp (수정)

#include "GA_AttackSelector.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h" // SendGameplayEventToActor
#include "GameplayEffectTypes.h" // FGameplayEventData
#include "RamdomItemDefense.h"

UGA_AttackSelector::UGA_AttackSelector()
{
	// 이 어빌리티는 Event.Attack.Perform 태그로 활성화됩니다. (BP에서 설정 필요)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_AttackSelector::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// (이하 기존 로직은 동일)
	// TriggerEventData가 유효한지 확인
	if (!TriggerEventData)
	{
		RID_LOG(FColor::Red, TEXT("GA_AttackSelector: TriggerEventData is NULL!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 시전자의 ASC를 가져옵니다.
	UAbilitySystemComponent* OwnerASC = ActorInfo->AbilitySystemComponent.Get();
	if (!OwnerASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 블루프린트에서 구현된 SelectAttackType 함수 호출하여 실행할 공격 태그 결정
	FGameplayEventData TempEventData = *TriggerEventData;
	FGameplayTag SelectedAttackTag = SelectAttackType(TempEventData, OwnerASC);

	// 유효한 태그가 반환되었는지 확인
	if (SelectedAttackTag.IsValid())
	{
		const AActor* ConstTargetActor = TriggerEventData->Target;
		AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);

		if (TargetActor)
		{
			// 결정된 공격 실행 이벤트 전송
			SendExecuteAttackEvent(TargetActor, SelectedAttackTag);
		}
		else
		{
			RID_LOG(FColor::Yellow, TEXT("GA_AttackSelector: TargetActor is NULL in TriggerEventData!"));
		}
	}
	else
	{
		RID_LOG(FColor::Orange, TEXT("GA_AttackSelector: SelectAttackType returned Invalid Tag. No attack executed."));
	}

	// 선택 로직 완료 후 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_AttackSelector::SendExecuteAttackEvent(AActor* TargetActor, FGameplayTag ExecuteTag)
{
	if (!TargetActor || !ExecuteTag.IsValid())
	{
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: FAILED VALIDATION. TargetActor is %s, ExecuteTag is %s"),
			TargetActor ? TEXT("VALID") : TEXT("NULL"),
			ExecuteTag.IsValid() ? TEXT("VALID") : TEXT("INVALID")
		);
		return;
	}

	FGameplayEventData Payload;
	Payload.Target = TargetActor;
	Payload.Instigator = GetAvatarActorFromActorInfo();

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: FAILED. AvatarActor is NULL!"));
		return;
	}

	// [ ★★★ UE_LOG 추가 (핵심) ★★★ ]
	UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_AttackSelector [%s]: Sending execute event: %s (Target: %s)"), *GetNameSafe(AvatarActor), *ExecuteTag.ToString(), *GetNameSafe(TargetActor));

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, ExecuteTag, Payload);
}