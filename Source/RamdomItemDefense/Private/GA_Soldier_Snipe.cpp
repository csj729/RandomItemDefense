#include "GA_Soldier_Snipe.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

UGA_Soldier_Snipe::UGA_Soldier_Snipe()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;
	VisualProjectileSpeed = 10000.0f;

	MuzzleSocketName = FName("MuzzleSocket");
	ProjectileSpawnSocketName = FName("MuzzleSocket");

	DamageBase = 300.0f;
	DamageCoefficient = 2.5f;
}

void UGA_Soldier_Snipe::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	if (!AvatarActor || !DamageEffectClass || !StunEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
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

	// 1. [리팩토링] 발사 위치/회전 계산 (드론 체크 로직 포함됨)
	FVector SpawnLocation;
	FRotator SpawnRotation;
	GetMuzzleTransform(AvatarActor, SpawnLocation, SpawnRotation, TargetActor.Get());

	float Distance = FVector::Dist(SpawnLocation, TargetActor->GetActorLocation());
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
		&UGA_Soldier_Snipe::OnImpact,
		TravelTime,
		false
	);
}

void UGA_Soldier_Snipe::OnImpact()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	if (TargetActor.IsValid())
	{
		// 1. [리팩토링] 데미지 적용
		bool bHit = ApplyDamageToTarget(ActorInfo, TargetActor.Get(), DamageBase, DamageCoefficient);

		// 2. 스턴 적용 (데미지 적용 성공 시, 혹은 독립적으로)
		if (bHit && StunEffectClass)
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());
			if (TargetASC)
			{
				TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ActorInfo->AbilitySystemComponent->MakeEffectContext());
			}
		}
	}

	EndAbility(Handle, ActorInfo, GetCurrentActivationInfo(), true, false);
}