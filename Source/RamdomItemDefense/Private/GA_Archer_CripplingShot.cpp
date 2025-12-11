// Source/RamdomItemDefense/Private/GA_Archer_CripplingShot.cpp
#include "GA_Archer_CripplingShot.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h" // GE 적용을 위해 필요
#include "ProjectileBase.h" 
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h"

UGA_Archer_CripplingShot::UGA_Archer_CripplingShot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;
	VisualProjectileSpeed = 2000.0f;

	MuzzleSocketName = FName("MuzzleSocket");
	ProjectileSpawnSocketName = FName("MuzzleSocket");

	DamageBase = 300.0f;
	DamageCoefficient = 1.5f;
}

void UGA_Archer_CripplingShot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	if (!AvatarActor || !DamageEffectClass || !StunEffectClass || !ArmorShredEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
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

	// [리팩토링] 위치 계산
	FVector SpawnLocation;
	FRotator SpawnRotation;
	GetMuzzleTransform(AvatarActor, SpawnLocation, SpawnRotation, TargetActor.Get());

	// 투사체 로직 (기존 동일)
	float Distance = FVector::Dist(SpawnLocation, TargetActor->GetActorLocation());
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? FMath::Max(0.01f, Distance / VisualProjectileSpeed) : 0.01f;

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
		&UGA_Archer_CripplingShot::OnImpact,
		TravelTime,
		false
	);
}

void UGA_Archer_CripplingShot::OnImpact()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	if (TargetActor.IsValid())
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());
		if (TargetASC)
		{
			// 1. [리팩토링] 데미지 적용
			ApplyDamageToTarget(ActorInfo, TargetActor.Get(), DamageBase, DamageCoefficient);

			// 2. 추가 효과 적용
			FGameplayEffectContextHandle ContextHandle = ActorInfo->AbilitySystemComponent->MakeEffectContext();

			// 스턴
			if (StunEffectClass)
				TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);

			// 방깎
			if (ArmorShredEffectClass)
			{
				UE_LOG(LogRamdomItemDefense, Warning, TEXT("[GA_Archer_CripplingShot] OnImpact: Applying ArmorShred to %s."), *GetNameSafe(TargetActor.Get()));
				TargetASC->ApplyGameplayEffectToSelf(ArmorShredEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}