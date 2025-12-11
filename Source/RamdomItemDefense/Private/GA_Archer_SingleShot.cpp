// Source/RamdomItemDefense/Private/GA_Archer_SingleShot.cpp
#include "GA_Archer_SingleShot.h"
#include "AbilitySystemComponent.h"
#include "ProjectileBase.h" 
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

UGA_Archer_SingleShot::UGA_Archer_SingleShot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.15f;
	VisualProjectileSpeed = 2500.0f;

	// BaseSkill의 MuzzleSocketName을 설정
	MuzzleSocketName = FName("MuzzleSocket");
	ProjectileSpawnSocketName = FName("MuzzleSocket"); // (필요 시 유지하되 Base 변수 사용 권장)

	DamageBase = 100.0f;
	DamageCoefficient = 0.8f;
}

void UGA_Archer_SingleShot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Base의 ActivateAbility 호출 (MuzzleFlash 처리 등)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	// 1. 유효성 검사 및 타겟 확인
	if (!AvatarActor || !DamageEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
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

	// 2. [리팩토링] 스폰 위치/회전 계산 (단 한 줄로 해결!)
	FVector SpawnLocation;
	FRotator SpawnRotation;
	GetMuzzleTransform(AvatarActor, SpawnLocation, SpawnRotation, TargetActor.Get());

	// 3. 거리 및 시간 계산
	float Distance = FVector::Dist(SpawnLocation, TargetActor->GetActorLocation());
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? FMath::Max(0.01f, Distance / VisualProjectileSpeed) : 0.01f;

	// 4. 투사체 스폰
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

	// 5. 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_Archer_SingleShot::OnImpact,
		TravelTime,
		false
	);
}

void UGA_Archer_SingleShot::OnImpact()
{
	if (TargetActor.IsValid())
	{
		// [리팩토링] 데미지 적용 로직 통합 호출 (복잡한 ASC 조회/계산 삭제)
		ApplyDamageToTarget(GetCurrentActorInfo(), TargetActor.Get(), DamageBase, DamageCoefficient);
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}