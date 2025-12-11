#include "GA_MagicFighter_ArcaneBind.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h" // GE 적용을 위해 필요
#include "RamdomItemDefenseCharacter.h"
#include "RamdomItemDefense.h"

UGA_MagicFighter_ArcaneBind::UGA_MagicFighter_ArcaneBind()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;

	DamageBase = 200.0f;
	DamageCoefficient = 0.5f;
}

void UGA_MagicFighter_ArcaneBind::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 유효성 검사
	if (!HasAuthority(&ActivationInfo) || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageEffectClass || !StunEffectClass)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_ArcaneBind: Effect Classes missing!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (!TargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. [리팩토링] 데미지 적용 (계산 + GE 생성 + Apply + 로그/치명타 처리 모두 포함)
	bool bApplied = ApplyDamageToTarget(ActorInfo, TargetActor, DamageBase, DamageCoefficient);

	if (bApplied)
	{
		// 3. 추가 효과(스턴) 적용
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (TargetASC)
		{
			FGameplayEffectContextHandle ContextHandle = ActorInfo->AbilitySystemComponent->MakeEffectContext();
			TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
			RID_LOG(FColor::Green, TEXT("GA_ArcaneBind: Stun Applied to %s"), *TargetActor->GetName());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}