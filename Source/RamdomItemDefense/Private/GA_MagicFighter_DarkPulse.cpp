// Source/RamdomItemDefense/Private/GA_MagicFighter_DarkPulse.cpp (수정)

#include "GA_MagicFighter_DarkPulse.h"
#include "RamdomItemDefenseCharacter.h"
#include "ProjectileBase.h" 
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" 
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RID_DamageStatics.h" 
#include "SoldierDrone.h" // [ ★★★ 코드 추가 ★★★ ]


UGA_MagicFighter_DarkPulse::UGA_MagicFighter_DarkPulse()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.2f;

	VisualProjectileSpeed = 1500.0f;
	ExplosionRadius = 300.0f;
	ProjectileSpawnSocketName = FName("MuzzleSocket"); // [ ★★★ 코드 추가 ★★★ ]

	DamageBase = 50.f;
	DamageCoefficient = 0.3f;
}

void UGA_MagicFighter_DarkPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	// 1. 유효성 검사
	if (!AvatarActor || !SourceASC || !HasAuthority(&ActivationInfo) || !DamageEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
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

	// 2. 스폰 위치/회전 계산
	TargetImpactLocation = TargetActor->GetActorLocation(); // 폭발 위치 저장
	FVector SpawnLocation = AvatarActor->GetActorLocation(); // 기본 위치

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

	// (핵심) 타겟의 현재 위치가 아닌, '미래 폭발 지점'을 향해 발사
	FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, TargetImpactLocation);

	// 3. 이동 시간 계산
	float Distance = FVector::Dist(SpawnLocation, TargetImpactLocation);
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? FMath::Max(0.01f, Distance / VisualProjectileSpeed) : 0.01f;

	// 4. "날아가는" 이펙트 스폰 
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
			// (수정) DarkPulse는 타겟이 아닌 '지점'으로 가야 하므로 유도를 끄거나,
			// HomingTarget을 타겟 액터로 설정합니다. (유도 사용으로 유지)
			VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		}

		// [ ★★★ 빙빙 도는 문제 해결 ★★★ ]
		VisualProjectile->SetLifeSpan(TravelTime);

		UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, SpawnLocation));
	}

	// 5. 이동 시간(TravelTime) 후에 Explode 함수를 호출하도록 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_MagicFighter_DarkPulse::Explode,
		TravelTime, // 계산된 이동 시간
		false
	);
}

void UGA_MagicFighter_DarkPulse::Explode()
{
	// 1. 현재 어빌리티 정보 가져오기
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	// 2. 시전자(Caster) ASC 및 AttributeSet 가져오기 (드론/캐릭터 공용)
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	const UMyAttributeSet* AttributeSet = CasterASC ? Cast<const UMyAttributeSet>(CasterASC->GetAttributeSet(UMyAttributeSet::StaticClass())) : nullptr;

	if (!CasterASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 폭발 이펙트 및 사운드 재생
	if (ExplosionEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, TargetImpactLocation);
	if (ExplosionSound) UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, TargetImpactLocation);

	// 4. 데미지 계산
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(CasterASC, true);
	float FinalDamage = BaseDamage;
	if (bDidCrit)
	{
		FinalDamage = BaseDamage * URID_DamageStatics::GetCritMultiplier(CasterASC);
	}

	// 5. 디버그 구체 그리기
	DrawDebugSphere(GetWorld(), TargetImpactLocation, ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);

	// 6. 범위 내 몬스터 찾기
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), TargetImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_DarkPulse Exploded! Damage: %.1f (Crit: %s), Hit %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 7. 찾은 몬스터들에게 데미지 적용
	if (DamageByCallerTag.IsValid())
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
		FGameplayEffectSpecHandle SpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);

			for (AActor* HitActor : OverlappedActors)
			{
				AMonsterBaseCharacter* HitMonster = Cast<AMonsterBaseCharacter>(HitActor);
				if (!HitMonster || HitMonster->IsDying()) continue;

				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitMonster);
				if (TargetASC)
				{
					// [데미지 적용]
					CasterASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

					// [치명타 텍스트] 
					if (bDidCrit)
					{
						URID_DamageStatics::OnCritDamageOccurred.Broadcast(HitMonster, FinalDamage);
					}

					// [슬로우 효과 적용]
					if (SlowEffectClass)
					{
						TargetASC->ApplyGameplayEffectToSelf(SlowEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_DarkPulse: FAILED (DamageByCallerTag IS INVALID. Check Blueprint!)"));
	}

	// 8. 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}