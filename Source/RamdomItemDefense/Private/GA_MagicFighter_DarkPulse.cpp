#include "GA_MagicFighter_DarkPulse.h"
#include "RamdomItemDefenseCharacter.h"
#include "ProjectileBase.h" 
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" 
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "RID_DamageStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"

UGA_MagicFighter_DarkPulse::UGA_MagicFighter_DarkPulse()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.2f;
	VisualProjectileSpeed = 1500.0f;
	ExplosionRadius = 300.0f;

	// BaseSkill 변수 활용
	MuzzleSocketName = FName("MuzzleSocket");
	ProjectileSpawnSocketName = FName("MuzzleSocket");

	DamageBase = 50.f;
	DamageCoefficient = 0.3f;
}

void UGA_MagicFighter_DarkPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	if (!AvatarActor || !HasAuthority(&ActivationInfo) || !DamageEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (!TargetActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. [리팩토링] 발사 위치/회전 계산
	FVector SpawnLocation;
	FRotator SpawnRotation;
	GetMuzzleTransform(AvatarActor, SpawnLocation, SpawnRotation, TargetActor.Get());

	// 나중에 폭발할 위치 저장
	TargetImpactLocation = TargetActor->GetActorLocation();

	float Distance = FVector::Dist(SpawnLocation, TargetImpactLocation);
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? FMath::Max(0.01f, Distance / VisualProjectileSpeed) : 0.01f;

	// 2. 투사체 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileBase* VisualProjectile = GetWorld()->SpawnActorDeferred<AProjectileBase>(
		ProjectileClass,
		FTransform(SpawnRotation, SpawnLocation),
		SpawnParams.Owner,
		SpawnParams.Instigator,
		SpawnParams.SpawnCollisionHandlingOverride
	);

	if (VisualProjectile)
	{
		if (VisualProjectile->ProjectileMovement)
		{
			VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		}
		VisualProjectile->SetLifeSpan(TravelTime);
		UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, SpawnLocation));
	}

	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_MagicFighter_DarkPulse::Explode,
		TravelTime,
		false
	);
}

void UGA_MagicFighter_DarkPulse::Explode()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	// 1. 이펙트 및 사운드
	if (ExplosionEffect)
	{
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
		{
			OwnerCharacter->Multicast_SpawnParticleAtLocation(ExplosionEffect, TargetImpactLocation, FRotator::ZeroRotator, FVector(1.0f));
		}
	}
	if (ExplosionSound) UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, TargetImpactLocation);

	// 2. [리팩토링] 데미지 Spec 생성 (범위 공격용)
	float FinalDamage = 0.f;
	bool bDidCrit = false;
	FGameplayEffectSpecHandle DamageSpecHandle = MakeDamageEffectSpec(ActorInfo, DamageBase, DamageCoefficient, FinalDamage, bDidCrit);

	// 3. 범위 탐색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), TargetImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("GA_DarkPulse Exploded! Damage: %.1f (Crit: %s), Hit %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 4. 적용 루프
	UGameplayEffect* SlowEffect = SlowEffectClass ? SlowEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

	if (DamageSpecHandle.IsValid())
	{
		for (AActor* HitActor : OverlappedActors)
		{
			AMonsterBaseCharacter* HitMonster = Cast<AMonsterBaseCharacter>(HitActor);
			if (!HitMonster || HitMonster->IsDying()) continue;

			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitMonster);
			if (TargetASC)
			{
				// 데미지
				ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);

				// 치명타 알림
				if (bDidCrit) URID_DamageStatics::OnCritDamageOccurred.Broadcast(HitMonster, FinalDamage);

				// 슬로우
				if (SlowEffect) TargetASC->ApplyGameplayEffectToSelf(SlowEffect, 1.0f, ContextHandle);
			}
		}
	}

	EndAbility(Handle, ActorInfo, GetCurrentActivationInfo(), true, false);
}