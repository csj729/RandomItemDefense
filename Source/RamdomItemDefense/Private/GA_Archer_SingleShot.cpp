#include "GA_Archer_SingleShot.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
#include "ProjectileBase.h" 
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "SoldierDrone.h" // [ ★★★ 코드 추가 ★★★ ]

UGA_Archer_SingleShot::UGA_Archer_SingleShot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.15f;
	VisualProjectileSpeed = 2500.0f;
	ProjectileSpawnSocketName = FName("MuzzleSocket"); // [ ★★★ 코드 추가 ★★★ ]

	DamageBase = 100.0f;
	DamageCoefficient = 0.8f;
}

void UGA_Archer_SingleShot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	// 1. 유효성 검사
	if (!AvatarActor || !SourceASC || !AvatarActor->HasAuthority() || !DamageEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
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

	// --- [ ★★★ 스폰 위치/회전 계산 로직 수정 (Snipe와 동일) ★★★ ] ---
	FVector TargetLocation = TargetActor->GetActorLocation();
	FVector SpawnLocation = AvatarActor->GetActorLocation();

	USceneComponent* AttachComponent = nullptr;
	if (ARamdomItemDefenseCharacter* CharacterActor = Cast<ARamdomItemDefenseCharacter>(AvatarActor))
	{
		AttachComponent = CharacterActor->GetMesh();
	}
	else if (ASoldierDrone* DroneActor = Cast<ASoldierDrone>(AvatarActor))
	{
		AttachComponent = DroneActor->GetMesh();
	}

	if (AttachComponent && ProjectileSpawnSocketName != NAME_None && AttachComponent->DoesSocketExist(ProjectileSpawnSocketName))
	{
		SpawnLocation = AttachComponent->GetSocketLocation(ProjectileSpawnSocketName);
	}

	FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, TargetLocation);
	// --- [ ★★★ 수정 끝 ★★★ ] ---


	float Distance = FVector::Dist(SpawnLocation, TargetLocation);
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

		// [ ★★★ 빙빙 도는 문제 해결 ★★★ ]
		VisualProjectile->SetLifeSpan(TravelTime);

		UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, SpawnLocation));
	}

	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_Archer_SingleShot::OnImpact,
		TravelTime,
		false
	);
}

/** 투사체 도착 시 실제 데미지를 적용하는 함수 */
void UGA_Archer_SingleShot::OnImpact()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!SourceASC || !TargetActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());

	// [ ★★★ 수정 ★★★ ] (드론/캐릭터 공용)
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));

	if (!TargetASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor.Get(), true);

	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (DamageSpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}