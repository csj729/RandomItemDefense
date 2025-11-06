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


UGA_MagicFighter_DarkPulse::UGA_MagicFighter_DarkPulse()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.2f;

	VisualProjectileSpeed = 1500.0f;
	ExplosionRadius = 300.0f;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	// 부모 클래스의 변수에 기본값 설정
	DamageBase = 50.f;
	DamageCoefficient = 0.3f; // (30%)
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
}

void UGA_MagicFighter_DarkPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageEffectClass || !ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());

	if (!OwnerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 시작/끝 위치, 이동 시간 계산
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
	FVector StartLocation = OwnerCharacter->GetActorLocation();
	TargetImpactLocation = TargetActor->GetActorLocation();

	float Distance = FVector::Dist(StartLocation, TargetImpactLocation);
	float TravelTime = 0.0f;
	if (VisualProjectileSpeed > 0.0f)
	{
		TravelTime = Distance / VisualProjectileSpeed;
	}

	// 4. "날아가는" 이펙트 스폰 
	if (ProjectileClass)
	{
		FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetImpactLocation);
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

	// 2. 시전자(Caster) ASC 및 AttributeSet 가져오기
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	const UMyAttributeSet* AttributeSet = OwnerCharacter ? OwnerCharacter->GetAttributeSet() : nullptr;

	if (!CasterASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 폭발 이펙트 및 사운드 재생
	if (ExplosionEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, TargetImpactLocation);
	if (ExplosionSound) UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, TargetImpactLocation);

	// --- [ ★★★ 코드 수정 ★★★ ] ---
	// 4. 데미지 계산 (부모 변수 사용)
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	// 하드코딩된 값 대신 부모의 변수(DamageBase, DamageCoefficient) 사용
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);
	// --- [ ★★★ 코드 수정 끝 ★★★ ] ---

	// 4-1. 치명타 굴림을 '단 한 번' 수행합니다. (bIsSkillAttack: true)
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(CasterASC, true);

	// 4-2. 치명타 여부에 따라 '최종 데미지'를 '단 한 번' 계산합니다.
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

	RID_LOG(FColor::Cyan, TEXT("GA_DarkPulse Exploded! Damage: %.1f (Crit: %s), Hit %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 7. 찾은 몬스터들에게 데미지 적용
	if (DamageByCallerTag.IsValid())
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

		// 7-1. 데미지 GE Spec을 '단 한 번' 생성합니다.
		FGameplayEffectSpecHandle SpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 7-2. Spec에 '최종 데미지'를 '단 한 번' 설정합니다.
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);

			for (AActor* HitActor : OverlappedActors)
			{
				AMonsterBaseCharacter* HitMonster = Cast<AMonsterBaseCharacter>(HitActor);
				if (!HitMonster || HitMonster->IsDying()) continue;

				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitMonster);
				if (TargetASC)
				{
					// 7-3. [데미지 적용] 미리 만들어둔 '동일한 Spec'을 모든 타겟에게 적용합니다.
					CasterASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

					// 7-4. [치명타 텍스트] 치명타가 발생했다면, 이 몬스터에게 델리게이트를 방송합니다.
					if (bDidCrit)
					{
						URID_DamageStatics::OnCritDamageOccurred.Broadcast(HitMonster, FinalDamage);
					}

					// 7-5. [슬로우 효과 적용]
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
		RID_LOG(FColor::Red, TEXT("GA_DarkPulse: FAILED (DamageByCallerTag IS INVALID. Check Blueprint!)"));
	}

	// 8. 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}