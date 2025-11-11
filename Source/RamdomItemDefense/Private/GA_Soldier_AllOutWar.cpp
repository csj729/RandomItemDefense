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
	BuffEffectComponent = nullptr;
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
			// 2-4. 지속 이펙트 스폰
			if (BuffEffect)
			{
				BuffEffectComponent = UGameplayStatics::SpawnEmitterAttached(
					BuffEffect,
					OwnerCharacter->GetMesh(),
					BuffEffectAttachSocketName,
					FVector(130.f, 0.f, -10.f), // Relative Location
					FRotator::ZeroRotator, // Relative Rotation
					BuffEffectScale,
					EAttachLocation::SnapToTarget,
					true // bAutoDestroy
				);
			}

			// 2-5. 버프 제거 감지 태스크 등록 (이 로직은 그대로 유지)
			UAbilityTask_WaitGameplayEffectRemoved* WaitEffectRemovedTask = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, UltimateBuffEffectHandle);
			WaitEffectRemovedTask->OnRemoved.AddDynamic(this, &UGA_Soldier_AllOutWar::OnBuffEffectRemoved);
			WaitEffectRemovedTask->ReadyForActivation();
		}
	}
}

void UGA_Soldier_AllOutWar::OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo)
{
	if (BuffEffectComponent && BuffEffectComponent->IsValidLowLevel())
	{
		BuffEffectComponent->DestroyComponent();
		BuffEffectComponent = nullptr;
		RID_LOG(FColor::Yellow, TEXT("GA_Soldier_AllOutWar: Buff Effect removed (Duration Ended)."));
	}

	// 버프가 종료되면 어빌리티를 정상 종료시킵니다.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Soldier_AllOutWar::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (BuffEffectComponent && BuffEffectComponent->IsValidLowLevel())
	{
		BuffEffectComponent->DestroyComponent();
		BuffEffectComponent = nullptr;
	}

	if (SourceASC)
	{
		// 어빌리티가 취소(Cancelled)된 경우, 적용했던 버프도 즉시 제거합니다.
		if (UltimateBuffEffectHandle.IsValid())
		{
			SourceASC->RemoveActiveGameplayEffect(UltimateBuffEffectHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}