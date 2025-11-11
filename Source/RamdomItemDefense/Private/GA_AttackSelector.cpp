// Private/GA_AttackSelector.cpp (수정)

#include "GA_AttackSelector.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h" 
#include "GameplayEffectTypes.h" 
#include "RamdomItemDefense.h"

UGA_AttackSelector::UGA_AttackSelector()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_AttackSelector::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// [ ★★★ UE_LOG 추가 (핵심) ★★★ ]
	UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_AttackSelector [%s]: ACTIVATED (Received 'Event.Attack.Perform')"), *GetNameSafe(ActorInfo->AvatarActor.Get()));


	if (!TriggerEventData)
	{
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_AttackSelector: TriggerEventData is NULL!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEventData TempEventData = *TriggerEventData;
	FGameplayTag SelectedAttackTag = SelectAttackType(TempEventData); // BP 함수 호출

	if (SelectedAttackTag.IsValid())
	{
		// [ ★★★ UE_LOG 추가 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_AttackSelector [%s]: BP selected attack: %s"), *GetNameSafe(ActorInfo->AvatarActor.Get()), *SelectedAttackTag.ToString());

		const AActor* ConstTargetActor = TriggerEventData->Target;
		AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);

		if (TargetActor)
		{
			SendExecuteAttackEvent(TargetActor, SelectedAttackTag);
		}
		else
		{
			// [ ★★★ UE_LOG 수정 ★★★ ]
			UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_AttackSelector: TargetActor is NULL in TriggerEventData!"));
		}
	}
	else
	{
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_AttackSelector: SelectAttackType returned Invalid Tag. No attack executed."));
	}

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