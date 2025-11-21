// Private/GA_Soldier_AllOutWar.cpp (수정)

#include "GA_Soldier_AllOutWar.h"
#include "RamdomItemDefense.h"	
#include "RamdomItemDefenseCharacter.h"
#include "MyAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"

UGA_Soldier_AllOutWar::UGA_Soldier_AllOutWar()
{
	AttackSpeedBuffTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Soldier.AttackSpeed"));
	BuffEffectAttachSocketName = FName("Root");
	BuffIsActiveTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Soldier.AllOutWar.Active"));
	BuffEffectScale = FVector(1.0f);
}

void UGA_Soldier_AllOutWar::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	// --- [ ★★★ 수정: UltimateStateEffectClass 체크 제거 ★★★ ] ---
	if (!OwnerCharacter || !SourceASC || !AllOutWarBuffEffectClass || !AttackSpeedBuffTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("AlloutWar Fail, Tag = %s"), *AttackSpeedBuffTag.ToString());
		RID_LOG(FColor::Red, TEXT("GA_Soldier_AllOutWar: Missing required components or classes/tags. AttackSpeedBuffTag is: %s"), *AttackSpeedBuffTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. (자가 버프) 30초 버프 GE 적용 (이 로직은 그대로 유지)
	FGameplayEffectSpecHandle BuffSpecHandle = SourceASC->MakeOutgoingSpec(AllOutWarBuffEffectClass, 1.0f, MakeEffectContext(Handle, ActorInfo));
	if (BuffSpecHandle.IsValid())
	{
		const float CasterAttackDamage = AttributeSet->GetAttackDamage();
		const float AttackSpeedBuffValue = 0.5f + (CasterAttackDamage * 0.001f);

		BuffSpecHandle.Data.Get()->SetSetByCallerMagnitude(AttackSpeedBuffTag, AttackSpeedBuffValue);

		UltimateBuffEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*BuffSpecHandle.Data.Get());
		RID_LOG(FColor::Green, TEXT("GA_Soldier_AllOutWar: Applied 30s Buff. AttackSpeed Bonus: %.2f"), AttackSpeedBuffValue);

		if (UltimateBuffEffectHandle.IsValid())
		{
			// [★★★ 수정 ★★★] 캐릭터에게 이펙트 실행 명령
			if (BuffEffect)
			{
				OwnerCharacter->Multicast_AddBuffEffect(
					BuffIsActiveTag,
					BuffEffect,
					BuffEffectAttachSocketName,
					FVector(130.f, 0.f, -10.f), // 기존 오프셋 유지
					BuffEffectScale
				);
			}

			// 제거 대기 태스크
			UAbilityTask_WaitGameplayEffectRemoved* Task = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, UltimateBuffEffectHandle);
			Task->OnRemoved.AddDynamic(this, &UGA_Soldier_AllOutWar::OnBuffEffectRemoved);
			Task->ReadyForActivation();
		}
	}
}

void UGA_Soldier_AllOutWar::OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo)
{
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(GetAvatarActorFromActorInfo());
	if (OwnerCharacter)
	{
		OwnerCharacter->Multicast_RemoveBuffEffect(BuffIsActiveTag);
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Soldier_AllOutWar::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (OwnerCharacter)
	{
		OwnerCharacter->Multicast_RemoveBuffEffect(BuffIsActiveTag);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}