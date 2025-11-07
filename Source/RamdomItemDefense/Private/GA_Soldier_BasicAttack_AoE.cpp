// Private/GA_Soldier_BasicAttack_AoE.cpp

#include "GA_Soldier_BasicAttack_AoE.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_Soldier_BasicAttack_AoE::UGA_Soldier_BasicAttack_AoE()
{
	AoERadius = 200.0f;
}

void UGA_Soldier_BasicAttack_AoE::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!DamageEffectClass || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());

	if (!OwnerCharacter || !SourceASC || !TargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UMyAttributeSet* AttributeSet = OwnerCharacter->GetAttributeSet();
	if (!AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector ImpactLocation = TargetActor->GetActorLocation();

	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = OwnerAttackDamage * DamageCoefficient;

	const bool bDidCrit = URID_DamageStatics::CheckForCrit(SourceASC, false);
	float FinalDamage = BaseDamage;
	if (bDidCrit)
	{
		FinalDamage = BaseDamage * URID_DamageStatics::GetCritMultiplier(SourceASC);
	}

	FGameplayEffectSpecHandle DamageSpecHandle;
	if (DamageDataTag.IsValid())
	{
		DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
		if (DamageSpecHandle.IsValid())
		{
			DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);
		}
	}

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, AoERadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	if (DamageSpecHandle.IsValid())
	{
		for (AActor* HitActor : OverlappedActors)
		{
			AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(HitActor);
			if (Monster && !Monster->IsDying())
			{
				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
				if (TargetASC)
				{
					TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
					if (bDidCrit)
					{
						URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
					}
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}