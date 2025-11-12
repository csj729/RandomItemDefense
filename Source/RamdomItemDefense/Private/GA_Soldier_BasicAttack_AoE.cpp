// Private/GA_Soldier_BasicAttack_AoE.cpp (수정)

#include "GA_Soldier_BasicAttack_AoE.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameplayTagContainer.h" 


UGA_Soldier_BasicAttack_AoE::UGA_Soldier_BasicAttack_AoE()
{
	AoERadius = 200.0f;
	// (솔져 궁극기 전용 광역 공격의 데미지 설정)
	DamageBase = 0.0f; // (예시: 기본 데미지 0)
	DamageCoefficient = 1.0f; // (예시: 계수 100%)
}

void UGA_Soldier_BasicAttack_AoE::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// (Super::ActivateAbility() 호출은 GA_BasicAttack의 로직을 실행하므로 여기서는 제거합니다)
	// Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

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

	// 데미지 계산 공식 변경
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);

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