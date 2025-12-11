#include "GA_Archer_ArrowShower.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RamdomItemDefenseCharacter.h"
#include "RamdomItemDefense.h"	
#include "RID_DamageStatics.h"

UGA_Archer_ArrowShower::UGA_Archer_ArrowShower()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.1f;
	Radius = 400.0f;
	Duration = 3.0f;
	TickInterval = 0.2f;
	DamageBase = 20.0f;
	DamageCoefficient = 0.1f;
}

void UGA_Archer_ArrowShower::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo) || !TriggerEventData || !TriggerEventData->Target || !DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타겟 위치 및 이펙트
	TargetLocation = TriggerEventData->Target.Get()->GetActorLocation();
	if (ArrowShowerEffect)
	{
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
		{
			float EmitterScaleFactor = Radius / 400.f;
			OwnerCharacter->Multicast_SpawnParticleAtLocation(ArrowShowerEffect, TargetLocation, FRotator::ZeroRotator, FVector(EmitterScaleFactor));
		}
	}

	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_ArrowShower: Started."));

	// 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(EffectTickTimerHandle, this, &UGA_Archer_ArrowShower::OnEffectTick, TickInterval, true);
	GetWorld()->GetTimerManager().SetTimer(EffectDurationTimerHandle, this, &UGA_Archer_ArrowShower::OnEffectEnd, Duration, false);
}

void UGA_Archer_ArrowShower::OnEffectTick()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority()) return;

	// 1. [리팩토링] 데미지 Spec 생성 (틱마다 호출하여 크리티컬을 매번 새로 굴림)
	float FinalDamage = 0.f;
	bool bDidCrit = false;
	FGameplayEffectSpecHandle DamageSpecHandle = MakeDamageEffectSpec(ActorInfo, DamageBase, DamageCoefficient, FinalDamage, bDidCrit);

	if (bDidCrit) UE_LOG(LogRamdomItemDefense, Log, TEXT("ArrowShower Crit: %.1f"), FinalDamage);

	// 2. 범위 탐색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), TargetLocation, Radius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	// 3. 적용
	UGameplayEffect* SlowEffectToApply = SlowEffectClass ? SlowEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(GetCurrentAbilitySpecHandle(), ActorInfo);

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
					// 슬로우
					if (SlowEffectToApply) TargetASC->ApplyGameplayEffectToSelf(SlowEffectToApply, 1.0f, ContextHandle);
					// 치명타
					if (bDidCrit) URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
				}
			}
		}
	}
}

void UGA_Archer_ArrowShower::OnEffectEnd()
{
	GetWorld()->GetTimerManager().ClearTimer(EffectTickTimerHandle);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Archer_ArrowShower::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EffectTickTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EffectDurationTimerHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}