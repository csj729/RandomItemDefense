#include "GA_MagicFighter_MeteorStrike.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RamdomItemDefense.h" 
#include "RamdomItemDefenseCharacter.h"
#include "RID_DamageStatics.h" 

UGA_MagicFighter_MeteorStrike::UGA_MagicFighter_MeteorStrike()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.1f;
	DamageBase = 100.0f;
	DamageCoefficient = 0.2f;
}

void UGA_MagicFighter_MeteorStrike::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (!TargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ImpactLocation = TargetActor->GetActorLocation();
	FVector SpawnLocation = ImpactLocation + FVector(0.0f, 0.0f, SpawnHeight);

	// Falling Effect (멀티캐스트)
	if (FallingEffect)
	{
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
		{
			OwnerCharacter->Multicast_SpawnParticleAtLocation(FallingEffect, SpawnLocation, FRotator::ZeroRotator, FVector(1.0f));
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle, this, &UGA_MagicFighter_MeteorStrike::Explode, FallDuration, false);
}

void UGA_MagicFighter_MeteorStrike::Explode()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	// 1. [리팩토링] 데미지 Spec 생성 (범위 공격용)
	float FinalDamage = 0.f;
	bool bDidCrit = false;
	FGameplayEffectSpecHandle DamageSpecHandle = MakeDamageEffectSpec(ActorInfo, DamageBase, DamageCoefficient, FinalDamage, bDidCrit);

	// 2. Explosion Effect
	if (ExplosionEffect)
	{
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
		{
			OwnerCharacter->Multicast_SpawnParticleAtLocation(ExplosionEffect, ImpactLocation, FRotator::ZeroRotator, FVector(1.0f));
		}
	}

	// 3. 범위 탐색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("Meteor Exploded! Damage: %.1f (Crit: %s), Found %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 4. 적용 루프
	if (DamageSpecHandle.IsValid())
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
		UGameplayEffect* StunEffect = StunEffectClass ? StunEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;

		for (AActor* TargetActor : OverlappedActors)
		{
			AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
			if (Monster && !Monster->IsDying())
			{
				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
				if (TargetASC)
				{
					// 데미지 Spec 적용
					ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);

					// 치명타 알림
					if (bDidCrit) URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);

					// 스턴
					if (StunEffect) TargetASC->ApplyGameplayEffectToSelf(StunEffect, 1.0f, ContextHandle);
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, GetCurrentActivationInfo(), true, false);
}