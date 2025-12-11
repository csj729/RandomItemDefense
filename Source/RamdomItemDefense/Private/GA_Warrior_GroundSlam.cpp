// Source/RamdomItemDefense/Private/GA_Warrior_GroundSlam.cpp
#include "GA_Warrior_GroundSlam.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MonsterBaseCharacter.h"
#include "RID_DamageStatics.h"
#include "RamdomItemDefenseCharacter.h"

UGA_Warrior_GroundSlam::UGA_Warrior_GroundSlam()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.15f;
	ExplosionRadius = 300.0f;

	DamageBase = 80.0f;
	DamageCoefficient = 0.6f;
}

void UGA_Warrior_GroundSlam::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo) || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageEffectClass || !SlowEffectClass)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Warrior_GroundSlam: Effect Classes missing!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector ImpactLocation = TriggerEventData->Target.Get()->GetActorLocation();

	// 이펙트 재생 (Multicast_SpawnParticleAtLocation 사용)
	if (ImpactEffect)
	{
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
		{
			OwnerCharacter->Multicast_SpawnParticleAtLocation(ImpactEffect, ImpactLocation, FRotator::ZeroRotator, FVector(1.0f));
		}
	}

	// 1. [리팩토링] 데미지 Spec 생성 (1회 계산)
	float FinalDamage = 0.f;
	bool bDidCrit = false;
	FGameplayEffectSpecHandle DamageSpecHandle = MakeDamageEffectSpec(ActorInfo, DamageBase, DamageCoefficient, FinalDamage, bDidCrit);

	// 2. 범위 탐색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("GA_Warrior_GroundSlam Exploded! Damage: %.1f (Crit: %s), Hit %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 3. 루프 적용
	UGameplayEffect* SlowEffect = SlowEffectClass->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

	for (AActor* TargetActor : OverlappedActors)
	{
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
		if (Monster && !Monster->IsDying())
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
			if (TargetASC)
			{
				// [적용 1] 데미지 Spec 적용
				if (DamageSpecHandle.IsValid())
				{
					TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
				}

				// [적용 2] 치명타 텍스트
				if (bDidCrit)
				{
					URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
				}

				// [적용 3] 슬로우
				if (SlowEffect)
				{
					TargetASC->ApplyGameplayEffectToSelf(SlowEffect, 1.0f, ContextHandle);
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}