// Private/GA_Soldier_Grenade.cpp

#include "GA_Soldier_Grenade.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MonsterBaseCharacter.h"
#include "RID_DamageStatics.h"
// --- [ ★★★ 수정 ★★★ ] ---
#include "Kismet/GameplayStatics.h" // NiagaraFunctionLibrary -> GameplayStatics
#include "Particles/ParticleSystem.h" // 헤더 추가
// --- [ ★★★ 수정 끝 ★★★ ] ---

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
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_Grenade: DamageEffectClass or SlowEffectClass is not set in BP!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(CasterASC->GetAttributeSet(UMyAttributeSet::StaticClass()));

	if (!CasterASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector ImpactLocation = TriggerEventData->Target.Get()->GetActorLocation();

	// --- [ ★★★ 수정 ★★★ ] ---
	// 이펙트 스폰 (캐스케이드)
	if (ImpactEffect)
	{
		// UNiagaraFunctionLibrary::SpawnSystemAtLocation -> UGameplayStatics::SpawnEmitterAtLocation
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, ImpactLocation, FRotator::ZeroRotator, ImpactEffectScale, true);
	}
	// --- [ ★★★ 수정 끝 ★★★ ] ---

	// (이하 데미지 및 GE 적용 로직은 동일)
	const float CasterAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (CasterAttackDamage * DamageCoefficient);
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(CasterASC, true);
	float FinalDamage = BaseDamage;
	if (bDidCrit)
	{
		FinalDamage = BaseDamage * URID_DamageStatics::GetCritMultiplier(CasterASC);
	}

	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
	FGameplayEffectSpecHandle DamageSpecHandle;
	if (DamageEffectClass && DamageByCallerTag.IsValid())
	{
		DamageSpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (DamageSpecHandle.IsValid())
		{
			DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		}
	}

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("GA_Soldier_Grenade Exploded! Damage: %.1f (Crit: %s), Hit %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	UGameplayEffect* SlowEffect = SlowEffectClass->GetDefaultObject<UGameplayEffect>();

	for (AActor* TargetActor : OverlappedActors)
	{
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
		if (Monster && !Monster->IsDying())
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
			if (TargetASC)
			{
				if (DamageSpecHandle.IsValid()) TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
				if (bDidCrit) URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
				if (SlowEffect) TargetASC->ApplyGameplayEffectToSelf(SlowEffect, 1.0f, ContextHandle);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}