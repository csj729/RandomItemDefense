#include "GA_Soldier_Grenade.h"
#include "RamdomItemDefense.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MonsterBaseCharacter.h"
#include "RID_DamageStatics.h"

UGA_Soldier_Grenade::UGA_Soldier_Grenade()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.15f;
	ExplosionRadius = 300.0f;
	ImpactEffectScale = FVector(1.0f);

	DamageBase = 50.0f;
	DamageCoefficient = 0.5f;
}

void UGA_Soldier_Grenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo) || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageEffectClass || !SlowEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector ImpactLocation = TriggerEventData->Target.Get()->GetActorLocation();

	// 이펙트
	if (ImpactEffect)
	{
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
		{
			OwnerCharacter->Multicast_SpawnParticleAtLocation(ImpactEffect, ImpactLocation, FRotator::ZeroRotator, ImpactEffectScale);
		}
	}

	// 1. [리팩토링] 데미지 Spec 생성
	float FinalDamage = 0.f;
	bool bDidCrit = false;
	FGameplayEffectSpecHandle DamageSpecHandle = MakeDamageEffectSpec(ActorInfo, DamageBase, DamageCoefficient, FinalDamage, bDidCrit);

	// 2. 범위 탐색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("Grenade Exploded! Damage: %.1f, Hit %d"), FinalDamage, OverlappedActors.Num());

	// 3. 적용 루프
	UGameplayEffect* SlowEffect = SlowEffectClass->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

	if (DamageSpecHandle.IsValid())
	{
		for (AActor* TargetActor : OverlappedActors)
		{
			AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
			if (Monster && !Monster->IsDying())
			{
				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
				if (TargetASC)
				{
					// 데미지
					ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
					// 치명타 
					if (bDidCrit) URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
					// 슬로우
					if (SlowEffect) TargetASC->ApplyGameplayEffectToSelf(SlowEffect, 1.0f, ContextHandle);
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}