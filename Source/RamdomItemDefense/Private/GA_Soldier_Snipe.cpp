// Private/GA_Soldier_Snipe.cpp

#include "GA_Soldier_Snipe.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
#include "ProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UGA_Soldier_Snipe::UGA_Soldier_Snipe()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;
	VisualProjectileSpeed = 10000.0f;

	DamageBase = 300.0f;
	DamageCoefficient = 2.5f;
}

void UGA_Soldier_Snipe::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !DamageEffectClass || !StunEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
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

	FVector StartLocation = OwnerCharacter->GetActorLocation();
	FVector EndLocation = TargetActor->GetActorLocation();
	float Distance = FVector::Dist(StartLocation, EndLocation);
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? (Distance / VisualProjectileSpeed) : 0.0f;

	FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, EndLocation);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileBase* VisualProjectile = GetWorld()->SpawnActorDeferred<AProjectileBase>(
		ProjectileClass,
		FTransform(SpawnRotation, StartLocation),
		OwnerCharacter,
		OwnerCharacter,
		SpawnParams.SpawnCollisionHandlingOverride
	);

	if (VisualProjectile)
	{
		if (VisualProjectile->ProjectileMovement)
		{
			VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		}
		UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, StartLocation));
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
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!SourceASC || !TargetActor.IsValid() || !DamageEffectClass || !StunEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));

	if (!TargetASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor.Get(), true);

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();

	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
	if (DamageSpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}

	TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}