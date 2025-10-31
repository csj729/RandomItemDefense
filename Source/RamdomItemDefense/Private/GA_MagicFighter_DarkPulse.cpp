// Source/RamdomItemDefense/Private/GA_MagicFighter_DarkPulse.cpp

#include "GA_MagicFighter_DarkPulse.h"
#include "RamdomItemDefenseCharacter.h"
// --- [코드 수정] ---
// 시각 효과용 투사체를 다시 스폰할 것이므로 헤더를 포함합니다.
#include "DarkPulseProjectile.h"
// --- [코드 수정 끝] ---
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" // RID_LOG 사용을 위해

// 폭발 로직에 필요한 헤더들
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "DrawDebugHelpers.h"
// --- [코드 추가] ---
// 투사체 이동 컴포넌트 헤더 (변수 직접 접근 시 필요)
#include "GameFramework/ProjectileMovementComponent.h"
// --- [코드 추가 끝] ---


UGA_MagicFighter_DarkPulse::UGA_MagicFighter_DarkPulse()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.2f; // (1.0f로 설정하면 100% 발동)

	VisualProjectileSpeed = 1500.0f;
	ExplosionRadius = 300.0f;
}

void UGA_MagicFighter_DarkPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 서버인지, 필요한 데이터(EffectClass, Target)가 있는지 확인
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageEffectClass || !ProjectileClass)
	{
		//UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_DarkPulse: DamageEffectClass or ProjectileClass is not set in Blueprint."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TriggerEventData || !TriggerEventData->Target)
	{
		//UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_DarkPulse: TriggerEventData or Target is NULL."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 확률 체크
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());

	if (!OwnerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 시작/끝 위치, 이동 시간 계산
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor); // 스폰 시 HomingTarget으로 사용
	FVector StartLocation = OwnerCharacter->GetActorLocation();
	TargetImpactLocation = TargetActor->GetActorLocation(); // 멤버 변수에 저장

	float Distance = FVector::Dist(StartLocation, TargetImpactLocation);
	float TravelTime = 0.0f;
	if (VisualProjectileSpeed > 0.0f)
	{
		TravelTime = Distance / VisualProjectileSpeed;
	}

	// 4. "날아가는" 이펙트 스폰 (액터 스폰 방식으로 변경)
	if (ProjectileClass)
	{
		FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetImpactLocation);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerCharacter;
		SpawnParams.Instigator = OwnerCharacter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // 충돌 없이 항상 스폰

		// ADarkPulseProjectile을 스폰합니다. (이 투사체는 이제 '시각 효과'만 담당합니다)
		ADarkPulseProjectile* VisualProjectile = GetWorld()->SpawnActorDeferred<ADarkPulseProjectile>(
			ProjectileClass,
			FTransform(SpawnRotation, StartLocation),
			OwnerCharacter,
			OwnerCharacter,
			SpawnParams.SpawnCollisionHandlingOverride
		);

		if (VisualProjectile)
		{
			// --- [코드 수정] ---
			// GetProjectileMovement() 대신 ProjectileMovement 변수에 직접 접근합니다.
			if (VisualProjectile->ProjectileMovement)
			{
				VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
			}
			// --- [코드 수정 끝] ---

			UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, StartLocation));
			//RID_LOG(FColor::Cyan, TEXT("GA_DarkPulse: Spawned VISUAL projectile."));
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

	//RID_LOG(FColor::Green, TEXT("GA_DarkPulse: Timer SET. Waiting for Explode..."));
}

/** 타이머가 만료되었을 때 실제 폭발을 실행하는 함수 */
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

	// 3. 폭발 이펙트 및 사운드 재생 (저장된 TargetImpactLocation 사용)
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, TargetImpactLocation);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, TargetImpactLocation);
	}

	// 4. 데미지 계산
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float FinalDamage = -(50.f + (OwnerAttackDamage * 0.3f));

	// 5. 디버그 구체 그리기
	DrawDebugSphere(
		GetWorld(),
		TargetImpactLocation,
		ExplosionRadius,
		12,
		FColor::Red,
		false,
		2.0f,
		0,
		1.0f
	);

	// 6. 범위 내 몬스터 찾기
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		TargetImpactLocation,
		ExplosionRadius,
		ObjectTypes,
		AMonsterBaseCharacter::StaticClass(),
		{},
		OverlappedActors
	);

	RID_LOG(FColor::Cyan, TEXT("GA_DarkPulse Exploded (Delayed)! FinalDamage: %.1f, Hit %d monsters."), FinalDamage, OverlappedActors.Num());

	// 7. 찾은 몬스터들에게 데미지 적용
	if (DamageByCallerTag.IsValid())
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
		FGameplayEffectSpecHandle SpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, FinalDamage);

			for (AActor* HitActor : OverlappedActors)
			{
				AMonsterBaseCharacter* HitMonster = Cast<AMonsterBaseCharacter>(HitActor);
				if (!HitMonster || HitMonster->IsDying())
				{
					continue;
				}

				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitMonster);
				if (TargetASC)
				{
					// [데미지 적용]
					CasterASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

					// --- [ ★★★ 슬로우 효과 적용 추가 ★★★ ] ---
					if (SlowEffectClass)
					{
						// 데미지와 동일한 EffectContext를 사용하여 슬로우 GE를 적용합니다.
						TargetASC->ApplyGameplayEffectToSelf(SlowEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
					}
					// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
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